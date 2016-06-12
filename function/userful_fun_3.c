/*
 * Get number of arguments passed to function.
 */
#define PG_NARGS() (fcinfo->nargs)

在系统函数如下获取入参个数
Datum
dblink_connect(PG_FUNCTION_ARGS)

