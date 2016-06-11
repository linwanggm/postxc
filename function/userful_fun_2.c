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

2. 系统函数参数
	GISTENTRY  *entry = (GISTENTRY *) PG_GETARG_POINTER(0);
	TimeADT		query = PG_GETARG_TIMEADT(1);
	StrategyNumber strategy = (StrategyNumber) PG_GETARG_UINT16(2);
	
3. 调用系统函数
	const TimeADT *aa = (const TimeADT *) a;
	const TimeADT *bb = (const TimeADT *) b;

	return DatumGetBool(DirectFunctionCall2(time_lt,
											TimeADTGetDatumFast(*aa),
											TimeADTGetDatumFast(*bb)));
	：：系统函数time_lt，用DirectFunctionCallx调用，注意参数*aa, *bb类型，必须与time_lt系统函数参数类型一致。
	
	
