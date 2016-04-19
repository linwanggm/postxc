#include <stdio.h>
#include<libpq-fe.h>

int main(int argc,char *argv[])
{
    char pConnString[256];
    sprintf(pConnString,"dbname=db1 user=wln password=''");
    PGconn *conn;
    conn=PQconnectdb(pConnString);
    if(PQstatus(conn)!=CONNECTION_OK)
    {
      printf("connecterror!\n");
      PQfinish(conn);
      return -1;
        }
    PGresult *res;
    res=PQexec(conn,"select * from test");
    if(PQresultStatus(res) == PGRES_TUPLES_OK&& PQntuples(res)>0)
    {
      printf("c_id is %s\n", PQgetvalue(res,0,0));
      printf("c_name is : %s\n",PQgetvalue(res,0,1));
    }
    PQclear(res);
    PQfinish(conn);

        return 0;
}


----
编译
gcc -g -o libpq1 libpq1.c -I /home/wln/pg92/install/include  -I /home/wln/pg92/postgresql-9.2.0/src/include  \
  -L /home/wln/pg92/install/lib  -lpq
执行结果
wln@iZ232ngsvp8Z:~/2016/pg> ./libpq1
c_id is zhang
c_name is : 22
