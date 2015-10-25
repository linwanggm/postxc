
//pg_standby.c
pg_standby  --help
pg_standby allows PostgreSQL warm standby servers to be configured.

Usage:
  pg_standby [OPTION]... ARCHIVELOCATION NEXTWALFILE XLOGFILEPATH [RESTARTWALFILE]

Options:
  -c                 copy file from archive (default)
  -d                 generate lots of debugging output (testing only)
  -k NUMFILESTOKEEP  if RESTARTWALFILE is not used, remove files prior to limit
                     (0 keeps all)
  -l                 does nothing; use of link is now deprecated
  -r MAXRETRIES      max number of times to retry, with progressive wait
                     (default=3)
  -s SLEEPTIME       seconds to wait between file checks (min=1, max=60,
                     default=5)
  -t TRIGGERFILE     trigger file to initiate failover (no default)
  -V, --version      output version information, then exit
  -w MAXWAITTIME     max seconds to wait for a file (0=no limit) (default=0)
  -?, --help         show this help, then exit

Main intended use as restore_command in recovery.conf:
  restore_command = 'pg_standby [OPTION]... ARCHIVELOCATION %f %p %r'
e.g.
  restore_command = 'pg_standby /mnt/server/archiverdir %f %p %r'
  
测试:
wln@iZ232ngsvp8Z:~/pg92> pg_standby  -c  archivedir 000000010000000000000036 ./01  -d
Trigger file:         <not set>
Waiting for WAL file: 000000010000000000000036
WAL file path:        archivedir/000000010000000000000036
Restoring to:         ./01
Sleep interval:       5 seconds
Max wait interval:    0 forever
Command for restore:  cp "archivedir/000000010000000000000036" "./01"
Keep archive history: no cleanup required
running restore:      OK



wln@iZ232ngsvp8Z:~/pg92> pg_standby  -c  archivedir 000000010000000000000036  ./01 000000010000000000000035  -d
Trigger file:         <not set>
Waiting for WAL file: 000000010000000000000036
WAL file path:        archivedir/000000010000000000000036
Restoring to:         ./01
Sleep interval:       5 seconds
Max wait interval:    0 forever
Command for restore:  cp "archivedir/000000010000000000000036" "./01"
Keep archive history: 000000010000000000000035 and later
running restore:      OK

removing file "archivedir/000000010000000000000033"
removing file "archivedir/000000010000000000000034"


wln@iZ232ngsvp8Z:~/pg92> pg_standby  -c  archivedir 000000010000000000000035 ./01  000000010000000000000035 -d -t trigger
Trigger file:         trigger
Waiting for WAL file: 000000010000000000000035
WAL file path:        archivedir/000000010000000000000035
Restoring to:         ./01
Sleep interval:       5 seconds
Max wait interval:    0 forever
Command for restore:  cp "archivedir/000000010000000000000035" "./01"
Keep archive history: 000000010000000000000035 and later
WAL file not present yet. Checking for trigger file...
WAL file not present yet. Checking for trigger file...
WAL file not present yet. Checking for trigger file...
trigger file found: smart failover
随后在执行命令的当前目录下执行touch trigger

1.如果WAL file not present yet 输出时ctrl+c，则触发函数sighandler令signaled = true，在for循环中走如下代码：
		if (signaled)
		{
			Failover = FastFailover;
			if (debug)
			{
				fprintf(stderr, "signaled to exit: fast failover\n");
				fflush(stderr);
			}
		}
在输出内容退出信息，这里的failover 是指用户中断循环程序退出。

2.WAL file not present yet 输出时ctrl+\, 则signal(SIGQUIT, sigquit_handler) 触发函数sigquit_handler，内容：
/* We don't want SIGQUIT to core dump */
static void
sigquit_handler(int sig)
{
	signal(SIGINT, SIG_DFL);
	kill(getpid(), SIGINT);
}
可以看出执行这个函数中断该循环的执行。

3.触发fast failover有如下几种：

	//trigger内容开始部分为fast
		if (strncmp(buf, "fast", 4) == 0)
	{
		Failover = FastFailover;

		fprintf(stderr, "trigger file found: fast failover\n");
		fflush(stderr);
		...
	}
	上面的ctrl+c
	指定时间内没找到归档日志也是fast failover
	
4. 触发为smart failover 有如下场景：
     //trigger文件为空
	if (stat_buf.st_size == 0)
	{
		Failover = SmartFailover;
		fprintf(stderr, "trigger file found: smart failover\n");
		fflush(stderr);
		return;
	}
    //trigger 开始部分为smart
	if (strncmp(buf, "smart", 5) == 0)
	{
		Failover = SmartFailover;
		fprintf(stderr, "trigger file found: smart failover\n");
		fflush(stderr);
		close(fd);
		return;
	}

5.fast failover 和smart failover有什么区别？
  想起postgres数据库停止的几种模式，如果是fast stop则表示发送sigint信号，如果是smart stop则发送sigterm信号，没做完的还是允许进程做完。
  这里的区别是什么？
  	if (Failover == FastFailover)
		exit(1);

	if (CustomizableNextWALFileReady())
   可以看出，如果是fast failover则直接退出，如果是smart failover则会正常执行完（该cp则cp，该remover指定之前的文件就rm,执行完毕才退出）
   示例：
 wln@iZ232ngsvp8Z:~/pg92> pg_standby  -c  archivedir_1 00000001000000000000004B  ./01  00000001000000000000004B -d -t trigger
Trigger file:         trigger
Waiting for WAL file: 00000001000000000000004B
WAL file path:        archivedir_1/00000001000000000000004B
Restoring to:         ./01
Sleep interval:       5 seconds
Max wait interval:    0 forever
Command for restore:  cp "archivedir_1/00000001000000000000004B" "./01"
Keep archive history: 00000001000000000000004B and later
trigger file found: fast failover
wln@iZ232ngsvp8Z:~/pg92> ll 01
total 0
wln@iZ232ngsvp8Z:~/pg92> ll 
total 20736
drwxr-xr-x  2 wln users     4096 Oct 25 09:56 01
drwxr-xr-x  2 wln users     4096 Oct 25 09:54 archivedir
drwxr-xr-x  2 wln users     4096 Oct 25 09:54 archivedir_1
drwx------ 15 wln users     4096 Oct 25 09:54 data
-rw-r--r--  1 wln users     1483 Oct 24 22:21 ha.sh
drwxr-xr-x  6 wln users     4096 Sep 20 10:14 install
-rwxr-xr-x  1 wln users    60691 Oct 25 08:34 pg_standby
drwxr-xr-x  7 wln users     4096 Sep 20 09:12 postgresql-9.2.0
-rw-r--r--  1 wln users 21068595 Sep 13 20:53 postgresql-9.2.0.tar.gz
-rw-r--r--  1 wln users     4779 Oct 24 15:26 recovery.conf
-rw-r--r--  1 wln users       85 Oct 24 16:33 re.sh
drwx------ 15 wln users     4096 Oct 25 09:47 standby
-rwxr-xr-x  1 wln users    13745 Oct 25 00:04 t1
-rw-r--r--  1 wln users      698 Oct 25 00:03 t1.c
-rw-r--r--  1 wln users        0 Oct 25 09:56 trigger
-rw-r--r--  1 wln users        5 Oct 25 09:56 trigger_1
wln@iZ232ngsvp8Z:~/pg92> ll archivedir_1
total 147664
-rw------- 1 wln users 16777216 Oct 25 09:54 000000010000000000000047
-rw------- 1 wln users 16777216 Oct 25 09:54 000000010000000000000048
-rw------- 1 wln users      292 Oct 25 09:54 000000010000000000000048.00000020.backup
-rw------- 1 wln users 16777216 Oct 25 09:54 000000010000000000000049
-rw------- 1 wln users      292 Oct 25 09:54 000000010000000000000049.00000020.backup
-rw------- 1 wln users 16777216 Oct 25 09:54 00000001000000000000004A
-rw------- 1 wln users 16777216 Oct 25 09:54 00000001000000000000004B
-rw------- 1 wln users      292 Oct 25 09:54 00000001000000000000004B.00000020.backup
-rw------- 1 wln users 16777216 Oct 25 09:54 00000001000000000000004C
-rw------- 1 wln users      292 Oct 25 09:54 00000001000000000000004C.00000020.backup
-rw------- 1 wln users 16777216 Oct 25 09:54 00000001000000000000004D
-rw------- 1 wln users      292 Oct 25 09:54 00000001000000000000004D.00000020.backup
-rw------- 1 wln users 16777216 Oct 25 09:54 00000001000000000000004E
-rw------- 1 wln users      292 Oct 25 09:54 00000001000000000000004E.00000020.backup
-rw------- 1 wln users 16777216 Oct 25 09:54 00000001000000000000004F
-rw------- 1 wln users      292 Oct 25 09:54 00000001000000000000004F.00000020.backup
wln@iZ232ngsvp8Z:~/pg92> ll 01
total 0
wln@iZ232ngsvp8Z:~/pg92> pg_standby  -c  archivedir_1 00000001000000000000004B  ./01  00000001000000000000004B -d -t trigger
Trigger file:         trigger
Waiting for WAL file: 00000001000000000000004B
WAL file path:        archivedir_1/00000001000000000000004B
Restoring to:         ./01
Sleep interval:       5 seconds
Max wait interval:    0 forever
Command for restore:  cp "archivedir_1/00000001000000000000004B" "./01"
Keep archive history: 00000001000000000000004B and later
trigger file found: smart failover
running restore:      OK

removing file "archivedir_1/000000010000000000000048"
removing file "archivedir_1/000000010000000000000047"
removing file "archivedir_1/000000010000000000000049"
removing file "archivedir_1/00000001000000000000004A"
wln@iZ232ngsvp8Z:~/pg92> ll 01
total 16404
-rw------- 1 wln users 16777216 Oct 25 09:57 00000001000000000000004B


