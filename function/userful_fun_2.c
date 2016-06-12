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
：： 可以看出，第一个参数是pointer类型，第二个是time类型，第三个为uint16类型。
总之，PG_GETARG_xxx是根据对应的参数来设置的。
	
3. 调用系统函数
	const TimeADT *aa = (const TimeADT *) a;
	const TimeADT *bb = (const TimeADT *) b;

	return DatumGetBool(DirectFunctionCall2(time_lt,
											TimeADTGetDatumFast(*aa),
											TimeADTGetDatumFast(*bb)));
	：：系统函数time_lt，用DirectFunctionCallx调用，注意参数*aa, *bb类型，必须与time_lt系统函数参数类型一致。
	
4. 随机赋值
static char salt_chars[] =
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
mysalt[0] = salt_chars[random() & 0x3f];
mysalt[1] = salt_chars[random() & 0x3f];

5. cstring to text
	chkpass    *password = (chkpass *) PG_GETARG_POINTER(0);

	PG_RETURN_TEXT_P(cstring_to_text(password->password));
   :: 函数cstring_to_text()
   
6. text_to)cstring函数
   text_to_cstring_buffer函数
   
	text	   *a2 = PG_GETARG_TEXT_PP(1);
	char		str[9];

	text_to_cstring_buffer(a2, str, sizeof(str));
	PG_RETURN_BOOL(strcmp(a1->password, crypt(str, a1->password)) == 0);
	
	char	   *priv_type = text_to_cstring(priv_type_text);
	

7. 变量StringInfoData buf;
初始化initStringInfo(&buf);
resetStringInfo(StringInfo str)
appendStringInfoChar(&buf, '(');
appendStringInfo(&buf, ", ");
pfree(buf.data)

8.
#define NAMEDATALEN 64
#define MAXLINE 1024

