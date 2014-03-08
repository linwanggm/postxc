#####################################################################################
# author:waln Email: linwanggm@gmail.com
# date: 03.04.2014
# readme:use the script to test whether the data cluster installed is ok
######################################################################################

#!/usr/bin/evn python
import os, sys, getpass

global checkOK
#pt_coordinator: first is coordinator ip, second is port.....
pt_coordinator = ['192.168.17.130','5700',  '192.168.17.130','5702', '192.168.17.130','5704']
#pt_datanode: first is datanode ip, second is port
pt_datanode = ['192.168.17.130','5706',  '192.168.17.130','5708', '192.168.17.130','5710']
nodeNum = len(pt_coordinator)/2 + len(pt_datanode)/2
#path:given  the path of psql, but you can set path=''
path='/home/wln/pgxc/install/bin'
user = getpass.getuser()

##
# function checkPgxcNode(): check the number of nodes on coordinators is right
##
def checkPgxcNode():
    global checkOK,string
    i = 0
    cnNum = len(pt_coordinator)/2
    string = 'check pgxc_node'
    print '#'*15 + ' ' * ((20-len(string))/2) + string + ' ' * ((20-len(string))/2) + '#'*15
    while i < cnNum:
        checkOK = False
        print pt_coordinator[2*i]
        cmd = ''
        if len(path) == 0:
            cmd = cmd + path + '/'
        cmd = cmd + "psql -d postgres -p " + pt_coordinator[2*i+1] + " -h " + pt_coordinator[2*i] + " -c 'select count(*) from pgxc_node';"
        file = os.popen(cmd)
        returnOut = file.read().split('\n')
        if int(returnOut[2]) != nodeNum:
            print pt_coordinator[2*i] + ' : pgxc_node isn\'t right'
            return
        i = i + 1 
    print '-' *5 + ' pgxc_node check ok. ' + '-' * 5 + '\n'
    checkOK = True

##
# function checkCreateTable():check all the coordinators can create table
##
def checkCreateTable():
    global checkOK,string
    if checkOK == False:
        return
    i = 0
    cnNum = len(pt_coordinator)/2
    string = 'check create table'
    print '#'*15 + ' ' * ((20-len(string))/2) + string + ' ' * ((20-len(string))/2) + '#'*15
    while i < cnNum:
        checkOK = False
        print pt_coordinator[2*i]
        cmd = ''
        if len(path) == 0:
            cmd = cmd + path + '/'
        cmd = cmd + "psql -d postgres -p " + pt_coordinator[2*i+1] + " -h " + pt_coordinator[2*i] + " -c 'create table test_t" + str(i) + "(id int) distribute by replication;'"
        file = os.popen(cmd)
        returnOut = file.read().split('\n')
        if returnOut[0] != "CREATE TABLE":
            print pt_coordinator[2*i] + ' : create table Error!'
            return
        i = i + 1
    print '-' *5 + ' create table ok. ' + '-' * 5 + '\n'
    checkOK = True

##
# function checkDropTable(): check whether all coordinators can drop table
##
def checkDropTable():
    global checkOK,string
    if checkOK == False:
        return
    i = 0
    cnNum = len(pt_coordinator)/2
    string = 'check drop table'
    print '#'*15 + ' ' * ((20-len(string))/2) + string + ' ' * ((20-len(string))/2) + '#'*15
    while i < cnNum:
        checkOK = False
        print pt_coordinator[2*i]
        cmd = ''
        if len(path) == 0:
            cmd = cmd + path + '/'
        cmd = cmd + "psql -d postgres -p " + pt_coordinator[2*i+1] + " -h " + pt_coordinator[2*i] + " -c 'drop table test_t" + str(i) + ";'"
        file = os.popen(cmd)
        returnOut = file.read().split('\n')
        if returnOut[0] != "DROP TABLE":
            print pt_coordinator[2*i] + ' : drop table Error!'
            return
        i = i + 1
    print '-' *5 + ' drop table ok. ' + '-' * 5 + '\n'
    checkOK = True

##
# function checkNodeSelect(); check execute select on nodes right
##
def checkNodeSelect(nodeList,nodeName,value):
    global checkOK,string
    if checkOK == False:
        return
    nodei = 0
    nodeNum = len(nodeList)/2
    cnNum = len(pt_coordinator)/2
    string = 'check select on'
    print '#'*15 + ' ' * ((20-len(string))/2) + string + ' ' * ((20-len(string))/2) + '#'*15
    while nodei < nodeNum:
        print nodeList[2*nodei]
        tbj = 0
        while tbj < cnNum:
            checkOK = False
            cmd = ''
            if len(path) == 0:
                cmd = cmd + path + '/'
            cmd = cmd + "psql -d postgres -p " + nodeList[2*nodei+1] + " -h " + nodeList[2*nodei] + " -c 'select count(1) from test_t" + str(tbj) + ";'"
            file = os.popen(cmd)
            returnOut = file.read().split('\n')
            if len(returnOut) < 2 or int(returnOut[2]) != value:
                print pt_datanode[2*i] + ' : ' + nodeList + ' select Error!'
                return
            tbj = tbj + 1
        nodei = nodei + 1
    print '-' *5 + ' ' + nodeName + ' select ok. ' + '-' * 5 + '\n'
    checkOK = True

##
# function checkInsert(): check all the coordinators can insert
##
def checkInsert(value):
    global checkOK,showStr
    if checkOK == False:
        return
    i = 0
    cnNum = len(pt_coordinator)/2
    string = 'check insert on'
    print '#'*15 + ' ' * ((20-len(string))/2) + string + ' ' * ((20-len(string))/2) + '#'*15
    while i < cnNum:
        checkOK = False
        print pt_coordinator[2*i]
        cmd = ''
        if len(path) == 0:
            cmd = cmd + path + '/'
        cmd = cmd + "psql -d postgres -p " + pt_coordinator[2*i+1] + " -h " + pt_coordinator[2*i] + " -c 'insert into test_t" + str(i) + " select generate_series(1," + str(value) +");'"
        file = os.popen(cmd)
        returnOut = file.read().split('\n')
        if returnOut[0] != "INSERT 0 " + str(value):
            print pt_coordinator[2*i] + ' : coordinator insert Error!'
            return
        i = i + 1
    print '-' *5 + ' insert ok. ' + '-' * 5 + '\n'
    checkOK = True
    
##
# function noCheckDropTable()
##
def noCheckDropTable():
    global checkOK,string
    if checkOK == True:
        return
    i = 0
    cnNum = len(pt_coordinator)/2
    string = 'no check drop table'
    print '#'*15 + ' ' * ((20-len(string))/2) + string + ' ' * ((20-len(string))/2) + '#'*15
    while i < cnNum:
        checkOK = False
        print pt_coordinator[2*i]
        cmd = ''
        if len(path) == 0:
            cmd = cmd + path + '/'
        cmd = cmd + "psql -d postgres -p " + pt_coordinator[2*i+1] + " -h " + pt_coordinator[2*i] + " -c 'drop table test_t" + str(i) + ";'"
        file = os.popen(cmd)
        i = i + 1



##
# function main()
##

if __name__ == '__main__':
    cnNum = len(pt_coordinator)
    dnNum = len(pt_datanode)
    if cnNum%2 == 1 or dnNum %2 == 1:
        print 'alter,node list is wrong.' 
        exit(1)
	# check pgxc_node on all coordinators are all ok
    checkPgxcNode()
	# check create table 
    checkCreateTable()
	#checkInsert(value): value stands for how many rows insert into tables
    checkInsert(100)
	#checkNodeSelect(pt_list,the kind of node,how many rows that insert into table)
    checkNodeSelect(pt_coordinator,'coordinator',100)
    checkNodeSelect(pt_datanode,'datanode',100)
	#check drop table on coordinator
    checkDropTable()  
	#when the cluster install ok, the function noCheckDropTable no need execute,
	# if cluster install not ok, the function noCheckDropTable will execute
    noCheckDropTable() 
