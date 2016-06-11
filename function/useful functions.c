熟悉掌握PostgreSQL 数据库常用函数，有助于快速进行数据库内核开发。这里列出经常用到的PostgreSQL数据库自定义函数，以便于以后查阅。
内容来自postgres-xc/contrib/adminpack

	if (is_absolute_path(filename))
	{
		/* Disallow '/a/b/data/..' */
		if (path_contains_parent_reference(filename))
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
			(errmsg("reference to parent directory (\"..\") not allowed"))));

		/*
		 * Allow absolute paths if within DataDir or Log_directory, even
		 * though Log_directory might be outside DataDir.
		 */
		if (!path_is_prefix_of_path(DataDir, filename) &&
			(!logAllowed || !is_absolute_path(Log_directory) ||
			 !path_is_prefix_of_path(Log_directory, filename)))
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 (errmsg("absolute path not allowed"))));
	}
	else if (!path_is_relative_and_below_cwd(filename))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 (errmsg("path must be in or below the current directory"))));
	
	::分析
	(1) is_absolute_path :: 判断给定的路径是否是绝对路径，返回值bool,true/false
	(2) path_contains_parent_reference()  :: 判断所给路径是否包含“..” 这样带父路径
	(3)path_is_prefix_of_path(path1, path2)  :: 判断path1 是否是path2的子字符串（从最前面开始）
	(4) path_is_relative_and_below_cwd ::  判断是否为相对路径且对应当前目录
	
	
		if (!superuser())
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
			  (errmsg("only superuser may access generic file functions"))));
	：： 分析
	superuser() 判断是否为超级用户（实质为判断用户uid）
	
			struct stat fst;

		if (stat(filename, &fst) >= 0)
			ereport(ERROR,
					(ERRCODE_DUPLICATE_FILE,
					 errmsg("file \"%s\" exists", filename)));
					 
	：： stat 判断当前文件是否存在。
	
		if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();
		
		：： pg系统函数 一般这样写的
		Datum
pg_file_rename(PG_FUNCTION_ARGS)
，那么如何获取它的传入参数呢？ 如上，PG_ARGISNULL(0) 判断第0个参数是否为NULL，PG_ARGISNULL(1) 判断第一个参数是否为NULL。
那么PG_RETURN_NULL() 又是什么？该函数不是Datum类型的吗？我们看如下定义：
/* To return a NULL do this: */
#define PG_RETURN_NULL()  \
	do { fcinfo->isnull = true; return (Datum) 0; } while (0)
	

	if (access(fn1, W_OK) < 0)
	{
		ereport(WARNING,
				(errcode_for_file_access(),
				 errmsg("file \"%s\" is not accessible: %m", fn1)));

		PG_RETURN_BOOL(false);
	}
	
	::Linux access函数功能描述： 检查调用进程是否可以对指定的文件执行某种操作
	那么PG_RETURN_BOOL 这个又是什么？ 见fmgr.h文件
	
	/* To return a NULL do this: */
#define PG_RETURN_NULL()  \
	do { fcinfo->isnull = true; return (Datum) 0; } while (0)

/* A few internal functions return void (which is not the same as NULL!) */
#define PG_RETURN_VOID()	 return (Datum) 0

/* Macros for returning results of standard types */

#define PG_RETURN_DATUM(x)	 return (x)
#define PG_RETURN_INT32(x)	 return Int32GetDatum(x)
#define PG_RETURN_UINT32(x)  return UInt32GetDatum(x)
#define PG_RETURN_INT16(x)	 return Int16GetDatum(x)
#define PG_RETURN_CHAR(x)	 return CharGetDatum(x)
#define PG_RETURN_BOOL(x)	 return BoolGetDatum(x)
#define PG_RETURN_OID(x)	 return ObjectIdGetDatum(x)
#define PG_RETURN_POINTER(x) return PointerGetDatum(x)
#define PG_RETURN_CSTRING(x) return CStringGetDatum(x)
#define PG_RETURN_NAME(x)	 return NameGetDatum(x)
/* these macros hide the pass-by-reference-ness of the datatype: */
#define PG_RETURN_FLOAT4(x)  return Float4GetDatum(x)
#define PG_RETURN_FLOAT8(x)  return Float8GetDatum(x)
#define PG_RETURN_INT64(x)	 return Int64GetDatum(x)
/* RETURN macros for other pass-by-ref types will typically look like this: */
#define PG_RETURN_BYTEA_P(x)   PG_RETURN_POINTER(x)
#define PG_RETURN_TEXT_P(x)    PG_RETURN_POINTER(x)
#define PG_RETURN_BPCHAR_P(x)  PG_RETURN_POINTER(x)
#define PG_RETURN_VARCHAR_P(x) PG_RETURN_POINTER(x)
#define PG_RETURN_HEAPTUPLEHEADER(x)  PG_RETURN_POINTER(x)

		values[0] = timestampbuf;
		values[1] = palloc(strlen(fctx->location) + strlen(de->d_name) + 2);
		sprintf(values[1], "%s/%s", fctx->location, de->d_name);

		tuple = BuildTupleFromCStrings(funcctx->attinmeta, values);

		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
：：  BuildTupleFromCStrings - build a HeapTuple given user data in C string form.
 values is an array of C strings, one for each attribute of the return tuple.
 A NULL string pointer indicates we want to create a NULL field.
  这个在自定义系统函数经常用到，比如自定义函数返回3列，则可以在这里设置
	
	
	
	
	
	
	
