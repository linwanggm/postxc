###################################################################################################################
# author: waln, Emaile:linwanggm@gmail.com
# date  : 2014.02.24 
# modify: 
# everyone can use and alter this script by freedom for private purpose.
# read me: 
#  1. installPath : the folder which contains the folders that you compile the source code ./configure
#      make && make install geted
#  2. dataPath : which used to contain the datanode,coordinator,gtm,you should make sure the folder is existed
#  3. pgxcPortBase : the port arrangeed likes this, first coordinator, then datanode, last gtm, one of which takes 
#     two number,one for port,the other for pool_port
#####################################################################################################################


#!/bin/bash
installPath=/home/wln/pgxc/install_1_1
dataPath=/home/wln/pgxc/data
pgxcPortBase=5300
coordinatorNum=1
datanodeNum=2
pgxcIp=localhost
gtmIp=localhost
#gtmPort=5310
gtmPort=`expr $pgxcPortBase + $coordinatorNum \* 2 + $datanodeNum \* 2 + 2`
#startNumCN to make datanode,the num from different num.etc if startNumCN=2,you will get coordinator2,coordinator3,...
#startNumDN to make datanode,the num from different num.etc if startNumDN=2,you will get datanode2,datanode3,..
startNumCN=1
startNumDN=1


##
# funcion modify_parameter(): modify some parameters in the file of postgres
##
function modify_parameter()
{
    path=$1 
    port=$2
    #modify some paramters int the file of postgres
    sed -i "/^#listen_addresses/c\listen_addresses = '*'"                          $path/postgresql.conf > /dev/null
    sed -i "/^#port = 5432/c\port = $port"                                         $path/postgresql.conf > /dev/null
    sed -i "/^#pooler_port =/c\pooler_port = `expr $port + 1`"                     $path/postgresql.conf > /dev/null
    sed -i "/^#gtm_host =/c\gtm_host = $gtmIp"                                     $path/postgresql.conf > /dev/null
    sed -i "/^#gtm_port =/c\gtm_port = $gtmPort"                                   $path/postgresql.conf > /dev/null

    sed -i "/^#log_destination =/c\log_destination = 'stderr'"                     $path/postgresql.conf > /dev/null
    sed -i "/^#logging_collector =/c\logging_collector = on"                       $path/postgresql.conf > /dev/null
    sed -i "/^#log_directory =/c\log_directory = 'pg_log'"                         $path/postgresql.conf > /dev/null
    sed -i "/^#log_filename =/c\log_filename = 'postgresql-%Y-%m-%d.log'"          $path/postgresql.conf > /dev/null
    sed -i "/#log_line_prefix =/c\log_line_prefix = '%m %c %d %p %a %x %e'"        $path/postgresql.conf > /dev/null
    sed -i "/#log_min_messages =/c\log_min_messages = debug1"                      $path/postgresql.conf > /dev/null

    echo "host all all 0.0.0.0/0 trust" >> $path/pg_hba.conf
}

##
# function install(): install datanode,coordinator,gtm
##
function installcndn()
{
    i=$startNumCN
	coordinatorPort=$pgxcPortBase
	#install coordinator
	while [ $i -lt `expr $coordinatorNum + $startNumCN` ]
	do
	    $installPath/bin/initdb -D $dataPath/coordinator$i --nodename coordinator$i
	    #modify some paramters int the file of postgres
        modify_parameter $dataPath/coordinator$i  $coordinatorPort
	    coordinatorPort=`expr $coordinatorPort + 2`
        i=`expr $i + 1`
	done
	
	#install datanode
	i=$startNumDN
	datanodePortBase=`expr $pgxcPortBase + $coordinatorNum \* 2`
	datanodePort=$datanodePortBase
	#install datanode
	while [ $i -lt `expr $datanodeNum + $startNumDN` ]
	do
	    $installPath/bin/initdb -D $dataPath/datanode$i --nodename datanode$i
	    #modify some paramters int the file of postgres
        modify_parameter $dataPath/datanode$i  $datanodePort
        datanodePort=`expr $datanodePort + 2`
        i=`expr $i + 1`
	done
}

##
#function installgtm(): install gtm
##
function installgtm()
{
	#install gtm
	$installPath/bin/initgtm -D $dataPath/gtm -Z gtm
	sed -i "/^#listen_addresses/c\listen_addresses = '*'"         $dataPath/gtm/gtm.conf > /dev/null
    sed -i "/^port = 6666/c\port = $gtmPort"                      $dataPath/gtm/gtm.conf > /dev/null
	sed -i "/log_file/c\log_file = 'gtm.log'"                     $dataPath/gtm/gtm.conf > /dev/null
	sed -i "/log_min_messages/c\log_min_messages = WARNING"       $dataPath/gtm/gtm.conf > /dev/null
}


##
# function stop(): stop datanode,coordinator,gtm
##
function stop()
{
	i=$startNumDN
	#stop datanode
	while [ $i -lt `expr $datanodeNum + $startNumDN` ]
	do
	    $installPath/bin/pg_ctl stop -D $dataPath/datanode$i -m fast -l logfile  > /dev/null 2>&1
		i=`expr $i + 1`
	done
	
    i=$startNumCN
	#stop coordinator
	while [ $i -lt `expr $coordinatorNum + $startNumCN` ]
	do
	    $installPath/bin/pg_ctl stop -D $dataPath/coordinator$i -m fast -l logfile   > /dev/null 2>&1
		i=`expr $i + 1`
	done

	#stop gtm
	$installPath/bin/gtm_ctl stop -D $dataPath/gtm -Z gtm -m fast -l logfile
}

##
# function showstop(): show how to stop datanode,coordinator,gtm
##
function showstop()
{
	echo ""
	i=$startNumDN
	#stop datanode
	echo "########### stop datanode ###########"
	while [ $i -lt `expr $datanodeNum + $startNumDN` ]
	do
	    echo "$installPath/bin/pg_ctl stop -D $dataPath/datanode$i -m fast -l logfile"
		i=`expr $i + 1`
	done
	echo ""

    i=$startNumCN
	#stop coordinator
	echo "########### stop coordinator ###########"
	while [ $i -lt `expr $coordinatorNum + $startNumCN` ]
	do
	    echo "$installPath/bin/pg_ctl stop -D $dataPath/coordinator$i -m fast -l logfile"
		i=`expr $i + 1`
	done
        echo ""

	#stop gtm
	echo "########### stop gtm ###########"
	echo "$installPath/bin/gtm_ctl stop -D $dataPath/gtm -Z gtm -m fast -l logfile" 
    echo ""
}

##
# function start(): start gtm,coordinator,datanode
##
function start()
{
	i=$startNumDN
	#start datanode
	while [ $i -lt `expr $datanodeNum + $startNumDN` ]
	do
	    $installPath/bin/pg_ctl start -D $dataPath/datanode$i -Z datanode  -l logfile > /dev/null 2>&1
		i=`expr $i + 1`
	done
	
    i=$startNumCN
	#start coordinator
	while [ $i -lt `expr $coordinatorNum + $startNumCN` ]
	do
	    $installPath/bin/pg_ctl start -D $dataPath/coordinator$i -Z coordinator -l logfile  > /dev/null 2>&1
		i=`expr $i + 1`
	done

	#start gtm
	$installPath/bin/gtm_ctl start -D $dataPath/gtm -Z gtm -l logfile
}

##
# function showstart(): show how to start gtm,coordinator,datanode
##
function showstart()
{	
    echo ""
	i=$startNumDN
	#start datanode
	echo "########### start datanode ###########"
	while [ $i -lt `expr $datanodeNum + $startNumDN` ]
	do
	    echo "$installPath/bin/pg_ctl start -D $dataPath/datanode$i -Z datanode -l logfile"
		i=`expr $i + 1`
	done
	echo ""
	
    i=$startNumCN
	echo "########### start coordinator ###########"
	#start coordinator
	while [ $i -lt `expr $coordinatorNum + $startNumCN` ]
	do
	    echo "$installPath/bin/pg_ctl start -D $dataPath/coordinator$i -Z coordinator -l logfile"
		i=`expr $i + 1`
	done
	echo ""
	
	#start gtm
	echo "########### start gtm ###########"
	echo "$installPath/bin/gtm_ctl start -D $dataPath/gtm -Z gtm -l logfile" 
	echo ""
}

##
# function cleandb(): stop datanode,coordinator,gtm then remove the folders of them
##
function cleandb()
{
    #stop datanode,coordinator,gtm
	stop
	i=$startNumDN
	#clean datanode
	while [ $i -lt `expr $datanodeNum + $startNumDN` ]
	do
		rm -rf $dataPath/datanode$i
		i=`expr $i + 1`
	done
	
    i=$startNumCN
	#clean coordinator
	while [ $i -lt `expr $coordinatorNum + $startNumCN` ]
	do
		rm -rf $dataPath/coordinator$i
		i=`expr $i + 1`
	done
	

	#clean gtm
	rm -rf $dataPath/gtm
}

##
# function regist(): regist datanode,coordinator on all coordinators,first execute "delete from pgxc_node;" then regist
##
function regist()
{
    echo "########### regist nodes ###########" 
    sleep 3
    #get the string of regist datanode
    i=$startNumDN
	datanodePortBase=`expr $pgxcPortBase + $coordinatorNum \* 2`
	datanodePort=$datanodePortBase
	registDatanodeString=""
    while [ $i -lt `expr $datanodeNum + $startNumDN` ]
	do
		registDatanodeString=$registDatanodeString"create node datanode$i with(type=datanode,host=$pgxcIp,port=$datanodePort);"
		i=`expr $i + 1`
		datanodePort=`expr $datanodePort + 2`
	done
	
	#get the string of regist coordinator
	i=$startNumCN
	coordinatorPort=$pgxcPortBase
	registCoordinatorString=""
    while [ $i -lt `expr $coordinatorNum + $startNumCN` ]
	do
		registCoordinatorString=$registCoordinatorString"create node coordinator$i with(type=coordinator,host=$pgxcIp,port=$coordinatorPort);"
		i=`expr $i + 1`
		coordinatorPort=`expr $coordinatorPort + 2`
	done	
	
	registString=$registDatanodeString$registCoordinatorString"select pgxc_pool_reload();"
	i=$startNumCN
	coordinatorPort=$pgxcPortBase
    while [ $i -lt `expr $coordinatorNum + $startNumCN` ]
	do
        echo $registString
		$installPath/bin/psql -d postgres -p $coordinatorPort -c "delete from pgxc_node;$registString"            
		i=`expr $i + 1`
		coordinatorPort=`expr $coordinatorPort + 2`
	done
}

##
#show regist(): show how to regist
##
function showregist()
{
	echo ""
    #get the string of regist datanode
    i=$startNumDN
	datanodePortBase=`expr $pgxcPortBase + $coordinatorNum \* 2`
	datanodePort=$datanodePortBase
	echo "########### regist datanode ###########"
    while [ $i -lt `expr $datanodeNum + $startNumDN` ]
	do
		echo "create node datanode$i with(type=datanode,host=$pgxcIp,port=$datanodePort);"
		i=`expr $i + 1`
		datanodePort=`expr $datanodePort + 2`
	done
    echo ""
	
	#get the string of regist coordinator
	i=$startNumCN
	coordinatorPort=$pgxcPortBase
	echo "########### regist coordinator ###########"
    while [ $i -lt `expr $coordinatorNum + $startNumCN` ]
	do
		echo "create node coordinator$i with(type=coordinator,host=$pgxcIp,port=$coordinatorPort);"
		i=`expr $i + 1`
		coordinatorPort=`expr $coordinatorPort + 2`
	done	
    echo ""
	
	echo "########### reload the regist infomation ###########"
	echo "select pgxc_node_reload();"
	echo ""

}




##
# function killnode(): kill node of datanode,coordinator,gtm, and romove some files which involve
##



function killnode()
{
    #kill datanode
    i=$startNumDN	
    datanodePortBase=`expr $pgxcPortBase + $coordinatorNum \* 2`
	datanodePort=$datanodePortBase
    while [ $i -lt `expr $datanodeNum + $startNumDN` ]
	do
		ps ux | grep datanode$i | awk '{print $2}' | xargs kill -9  > /dev/null 2>&1
		rm -rf $dataPath/datanode$i/postmaster.pid
		rm -rf /tmp/.s.PGSQL.$datanodePort*
		i=`expr $i + 1`
		datanodePort=`expr $datanodePort + 2`
	done
	
    #kill coordinator
	coordinatorPort=$pgxcPortBase
    i=$startNumCN	
    while [ $i -lt `expr $coordinatorNum + $startNumCN` ]
	do
		ps ux | grep coordinator$i | awk '{print $2}' | xargs kill -9  > /dev/null  2>&1
		rm -rf $dataPath/coordinator$i/postmaster.pid
	    rm -rf /tmp/.s.PGSQL.$coordinatorPort*
		rm -rf /tmp/.s.PGPOOL.`expr $coordinatorPort + 1`*
		i=`expr $i + 1`
		coordinatorPort=`expr $coordinatorPort + 2`
	done	

	#kill gtm
	ps ux | grep gtm | awk '{print $2}' | xargs kill -9  > /dev/null 2>&1 
    rm -rf $dataPath/gtm/gtm.pid	
}

##
#function killpostgres(): kill the progress which contains postgres
##
function killpostgres()
{
    ps ux | grep postgres | awk '{print $2}' | xargs kill -9 > /dev/null 2>&1
}

##
#function showuse(): show how to use the script
##
function showuse()
{
    echo "the use show as list:"
    echo "  -all            --install the database cluster"
    echo "  -start          --start gtm, coordinator, datanode"
    echo "  -stop           --stop datanode, datanode, coordinator"
    echo "  -killnode       --kill progres of datanode, datanode, coordinator, then you can start the cluster use -start"
    echo "  -killpostgres   --kill the progress which contains 'postgres'"
    echo "  -showstart      --show how to start gtm, datanode, coordinator"
    echo "  -showstop       --show how to stop gtm, datanode, coordinator"
    echo "  -showregist     --show how to regist datanode, coordinator on coordinator"
}

if [ $1 == "-all" ] ; then 
    installcndn
	installgtm
	start
	regist
elif [ $1 == "-cleandb" ] ; then 
    cleandb
elif [ $1 == "-start" ] ; then 
    start
elif [ $1 == "-stop" ] ; then 
	stop
elif [ $1 == "-killnode" ] ; then 
    killnode
elif [ $1 == "-killpostgres" ]; then
    killpostgres
elif [ $1 == "-showstart" ] ; then 
    showstart
elif [ $1 == "-showstop" ] ; then 
   showstop
elif [ $1 == "-showregist" ] ; then 
   showregist
else
    showuse
fi
