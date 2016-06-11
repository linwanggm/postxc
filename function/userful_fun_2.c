来自postgres-x2-master/contrib/auth_delay
1. 函数pg_usleep ::指定sleep的微秒数
/*
 * pg_usleep --- delay the specified number of microseconds, but
 * stop waiting if a signal arrives.
 *
 * This replaces the non-signal-aware version provided by src/port/pgsleep.c.
 */
void
pg_usleep(long microsec)

：：对比linux， linux下为usleep(微秒数)， sleep(秒数)

