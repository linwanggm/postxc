说明：
1. 内容参考pg9.5源码src/include/access/htup_details.h
2. 参考《PostgreSQL数据库内核分析》P59
3. 需要加载contrib/pageinspect
4. 参考：http://my.oschina.net/Suregogo/blog/176279
         http://my.oschina.net/Suregogo/blog/595335?fromerr=4Ru4VQhW

HeadTupleHeaderData结构体：
struct HeapTupleHeaderData
{
	union
	{
		HeapTupleFields t_heap;
		DatumTupleFields t_datum;
	}			t_choice;

	ItemPointerData t_ctid;		/* current TID of this or newer tuple (or a
								 * speculative insertion token) */

	/* Fields below here must match MinimalTupleData! */

	uint16		t_infomask2;	/* number of attributes + various flags */

	uint16		t_infomask;		/* various flag bits, see below */

	uint8		t_hoff;			/* sizeof header incl. bitmap, padding */

	/* ^ - 23 bytes - ^ */

	bits8		t_bits[FLEXIBLE_ARRAY_MEMBER];	/* bitmap of NULLs */

	/* MORE DATA FOLLOWS AT END OF STRUCT */
};

通过pg_relation_filepath('tbname')找出其对应的表数据文件，通过hex 文件名 即可查看
下面简单画出该结构体示意图(由于hexdump每16个字节输出1行，所以下面也是16字节1行):

 xmin(4bytes)     xmax（4bytes）                            cmax(4bytes)                   cmin(4bytes)    ctid(4bytes+4bytes+..)
 ctid(..+4bytes)  t_infomask2(2bytes) t_informask(2bytes)   t_bits(1byte) t_hoff(1byte)  values(...)
 
 操作示例；
 wln@iZ232ngsvp8Z:~> psql
psql (9.5beta1)
Type "help" for help.

postgres=# \d
       List of relations
 Schema | Name | Type  | Owner 
--------+------+-------+-------
 public | c1   | table | wln
 public | t1   | table | wln
 public | t2   | table | wln
 public | t4   | table | wln
(4 rows)

postgres=# select * from t2;
 id1 | id2 | id3 | id4 | id5 | id6 
-----+-----+-----+-----+-----+-----
   1 |   2 |   3 |   4 |     |    
   2 |   2 |   3 |   4 |     |    
   3 |   2 |   3 |   4 |     |    
   4 |   2 |   3 |   4 |     |    
   5 |   2 |   3 |   4 |     |    
   6 |   2 |   3 |   4 |     |    
   7 |   2 |   3 |   4 |     |    
   8 |   2 |   3 |   4 |     |    
   9 |   2 |   3 |   4 |     |    
  11 |     |     |     |     |    
  12 |     |     |     |     |    
  13 |     |     |     |     |    
  14 |     |     |     |     |    
  15 |     |     |     |     |    
  16 |     |     |     |     |    
  11 |  12 |  13 |  14 |  15 |  16
(16 rows)

postgres=# select pg_relation_filepath('t2');
 pg_relation_filepath 
----------------------
 base/12974/41017
(1 row)

postgres=# select * from heap_page_items(get_raw_page('t2', 'main', 0));
 lp | lp_off | lp_flags | lp_len | t_xmin | t_xmax | t_field3 | t_ctid | t_infomask2 | t_infomask | t_hoff |  t_bits  | t_oid 
----+--------+----------+--------+--------+--------+----------+--------+-------------+------------+--------+----------+-------
  1 |   8152 |        1 |     40 |   1320 |      0 |        0 | (0,1)  |           6 |       2305 |     24 | 11110000 |      
  2 |   8112 |        1 |     40 |   1320 |      0 |        0 | (0,2)  |           6 |       2305 |     24 | 11110000 |      
  3 |   8072 |        1 |     40 |   1320 |      0 |        0 | (0,3)  |           6 |       2305 |     24 | 11110000 |      
  4 |   8032 |        1 |     40 |   1320 |      0 |        0 | (0,4)  |           6 |       2305 |     24 | 11110000 |      
  5 |   7992 |        1 |     40 |   1320 |      0 |        0 | (0,5)  |           6 |       2305 |     24 | 11110000 |      
  6 |   7952 |        1 |     40 |   1320 |      0 |        0 | (0,6)  |           6 |       2305 |     24 | 11110000 |      
  7 |   7912 |        1 |     40 |   1320 |      0 |        0 | (0,7)  |           6 |       2305 |     24 | 11110000 |      
  8 |   7872 |        1 |     40 |   1320 |      0 |        0 | (0,8)  |           6 |       2305 |     24 | 11110000 |      
  9 |   7832 |        1 |     40 |   1320 |      0 |        0 | (0,9)  |           6 |       2305 |     24 | 11110000 |      
 10 |   7792 |        1 |     40 |   1320 |   1321 |        0 | (0,10) |        8198 |       1281 |     24 | 11110000 |      
 11 |   7760 |        1 |     28 |   1332 |      0 |        0 | (0,11) |           6 |       2305 |     24 | 10000000 |      
 12 |   7728 |        1 |     28 |   1332 |      0 |        0 | (0,12) |           6 |       2305 |     24 | 10000000 |      
 13 |   7696 |        1 |     28 |   1332 |      0 |        0 | (0,13) |           6 |       2305 |     24 | 10000000 |      
 14 |   7664 |        1 |     28 |   1332 |      0 |        0 | (0,14) |           6 |       2305 |     24 | 10000000 |      
 15 |   7632 |        1 |     28 |   1332 |      0 |        0 | (0,15) |           6 |       2305 |     24 | 10000000 |      
 16 |   7600 |        1 |     28 |   1332 |      0 |        0 | (0,16) |           6 |       2305 |     24 | 10000000 |      
 17 |   7552 |        1 |     48 |   1333 |      0 |        0 | (0,17) |           6 |       2304 |     24 |          |      
(17 rows)

postgres=# \q
wln@iZ232ngsvp8Z:~> hexdump pg95/data/base/12974/41017 
0000000 0000 0000 ff20 12d2 0000 0000 005c 1d80
0000010 2000 2004 0529 0000 9fd8 0050 9fb0 0050
0000020 9f88 0050 9f60 0050 9f38 0050 9f10 0050
0000030 9ee8 0050 9ec0 0050 9e98 0050 9e70 0050
0000040 9e50 0038 9e30 0038 9e10 0038 9df0 0038
0000050 9dd0 0038 9db0 0038 9d80 0060 0000 0000
0000060 0000 0000 0000 0000 0000 0000 0000 0000
*
0001d80 0535 0000 0000 0000 0000 0000 0000 0000
0001d90 0011 0006 0900 0018 000b 0000 000c 0000
0001da0 000d 0000 000e 0000 000f 0000 0010 0000
0001db0 0534 0000 0000 0000 0000 0000 0000 0000
0001dc0 0010 0006 0901 0118 0010 0000 0000 0000
0001dd0 0534 0000 0000 0000 0000 0000 0000 0000
0001de0 000f 0006 0901 0118 000f 0000 0000 0000
0001df0 0534 0000 0000 0000 0000 0000 0000 0000
0001e00 000e 0006 0901 0118 000e 0000 0000 0000
0001e10 0534 0000 0000 0000 0000 0000 0000 0000
0001e20 000d 0006 0901 0118 000d 0000 0000 0000
0001e30 0534 0000 0000 0000 0000 0000 0000 0000
0001e40 000c 0006 0901 0118 000c 0000 0000 0000
0001e50 0534 0000 0000 0000 0000 0000 0000 0000
0001e60 000b 0006 0901 0118 000b 0000 0000 0000
0001e70 0528 0000 0529 0000 0000 0000 0000 0000
0001e80 000a 2006 0501 0f18 000a 0000 0002 0000
0001e90 0003 0000 0004 0000 0528 0000 0000 0000
0001ea0 0000 0000 0000 0000 0009 0006 0901 0f18
0001eb0 0009 0000 0002 0000 0003 0000 0004 0000
0001ec0 0528 0000 0000 0000 0000 0000 0000 0000
0001ed0 0008 0006 0901 0f18 0008 0000 0002 0000
0001ee0 0003 0000 0004 0000 0528 0000 0000 0000
0001ef0 0000 0000 0000 0000 0007 0006 0901 0f18
0001f00 0007 0000 0002 0000 0003 0000 0004 0000
0001f10 0528 0000 0000 0000 0000 0000 0000 0000
0001f20 0006 0006 0901 0f18 0006 0000 0002 0000
0001f30 0003 0000 0004 0000 0528 0000 0000 0000
0001f40 0000 0000 0000 0000 0005 0006 0901 0f18
0001f50 0005 0000 0002 0000 0003 0000 0004 0000
0001f60 0528 0000 0000 0000 0000 0000 0000 0000
0001f70 0004 0006 0901 0f18 0004 0000 0002 0000
0001f80 0003 0000 0004 0000 0528 0000 0000 0000
0001f90 0000 0000 0000 0000 0003 0006 0901 0f18
0001fa0 0003 0000 0002 0000 0003 0000 0004 0000
0001fb0 0528 0000 0000 0000 0000 0000 0000 0000
0001fc0 0002 0006 0901 0f18 0002 0000 0002 0000
0001fd0 0003 0000 0004 0000 0528 0000 0000 0000
0001fe0 0000 0000 0000 0000 0001 0006 0901 0f18
0001ff0 0001 0000 0002 0000 0003 0000 0004 0000
0002000
wln@iZ232ngsvp8Z:~> hexdump pg95/data/base/12974/41017  -d
0000000   00000   00000   65312   04818   00000   00000   00092   07552
0000010   08192   08196   01321   00000   40920   00080   40880   00080
0000020   40840   00080   40800   00080   40760   00080   40720   00080
0000030   40680   00080   40640   00080   40600   00080   40560   00080
0000040   40528   00056   40496   00056   40464   00056   40432   00056
0000050   40400   00056   40368   00056   40320   00096   00000   00000
0000060   00000   00000   00000   00000   00000   00000   00000   00000
*
0001d80   01333   00000   00000   00000   00000   00000   00000   00000
0001d90   00017   00006   02304   00024   00011   00000   00012   00000
0001da0   00013   00000   00014   00000   00015   00000   00016   00000
0001db0   01332   00000   00000   00000   00000   00000   00000   00000
0001dc0   00016   00006   02305   00280   00016   00000   00000   00000
0001dd0   01332   00000   00000   00000   00000   00000   00000   00000
0001de0   00015   00006   02305   00280   00015   00000   00000   00000
0001df0   01332   00000   00000   00000   00000   00000   00000   00000
0001e00   00014   00006   02305   00280   00014   00000   00000   00000
0001e10   01332   00000   00000   00000   00000   00000   00000   00000
0001e20   00013   00006   02305   00280   00013   00000   00000   00000
0001e30   01332   00000   00000   00000   00000   00000   00000   00000
0001e40   00012   00006   02305   00280   00012   00000   00000   00000
0001e50   01332   00000   00000   00000   00000   00000   00000   00000
0001e60   00011   00006   02305   00280   00011   00000   00000   00000
0001e70   01320   00000   01321   00000   00000   00000   00000   00000
0001e80   00010   08198   01281   03864   00010   00000   00002   00000
0001e90   00003   00000   00004   00000   01320   00000   00000   00000
0001ea0   00000   00000   00000   00000   00009   00006   02305   03864
0001eb0   00009   00000   00002   00000   00003   00000   00004   00000
0001ec0   01320   00000   00000   00000   00000   00000   00000   00000
0001ed0   00008   00006   02305   03864   00008   00000   00002   00000
0001ee0   00003   00000   00004   00000   01320   00000   00000   00000
0001ef0   00000   00000   00000   00000   00007   00006   02305   03864
0001f00   00007   00000   00002   00000   00003   00000   00004   00000
0001f10   01320   00000   00000   00000   00000   00000   00000   00000
0001f20   00006   00006   02305   03864   00006   00000   00002   00000
0001f30   00003   00000   00004   00000   01320   00000   00000   00000
0001f40   00000   00000   00000   00000   00005   00006   02305   03864
0001f50   00005   00000   00002   00000   00003   00000   00004   00000
0001f60   01320   00000   00000   00000   00000   00000   00000   00000
0001f70   00004   00006   02305   03864   00004   00000   00002   00000
0001f80   00003   00000   00004   00000   01320   00000   00000   00000
0001f90   00000   00000   00000   00000   00003   00006   02305   03864
0001fa0   00003   00000   00002   00000   00003   00000   00004   00000
0001fb0   01320   00000   00000   00000   00000   00000   00000   00000
0001fc0   00002   00006   02305   03864   00002   00000   00002   00000
0001fd0   00003   00000   00004   00000   01320   00000   00000   00000
0001fe0   00000   00000   00000   00000   00001   00006   02305   03864
0001ff0   00001   00000   00002   00000   00003   00000   00004   00000
0002000

可以看出，元组头长度均为24（xmin+xmax+cmin+cmax+ctid+t_infomask2+t_infomask+t_bits+t_hoff）,
元组中数据长度lp_len 为48,40,28。可以看出，有5列为空时长度最短28（24+4），无空列则为24+486=48。












