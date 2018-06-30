说明：下面使用函数需要加载contrib/pageinspect

表数据存储文件页（8K）包含存储结构如下：这个图示来自pg9.5源代码src/include/storage/bufpage.h中：
/*
 * A postgres disk page is an abstraction layered on top of a postgres
 * disk block (which is simply a unit of i/o, see block.h).
 *
 * specifically, while a disk block can be unformatted, a postgres
 * disk page is always a slotted page of the form:
 *
 * +----------------+---------------------------------+
 * | PageHeaderData | linp1 linp2 linp3 ...           |
 * +-----------+----+---------------------------------+
 * | ... linpN |									  |
 * +-----------+--------------------------------------+
 * |		   ^ pd_lower							  |
 * |												  |
 * |			 v pd_upper							  |
 * +-------------+------------------------------------+
 * |			 | tupleN ...                         |
 * +-------------+------------------+-----------------+
 * |	   ... tuple3 tuple2 tuple1 | "special space" |
 * +--------------------------------+-----------------+
 *									^ pd_special
 *
 * a page is full when nothing can be added between pd_lower and
 * pd_upper.
 *
 * all blocks written out by an access method must be disk pages.
 *
 * EXCEPTIONS:
 *
 * obviously, a page is not formatted before it is initialized by
 * a call to PageInit.
 *
 * NOTES:
 *
 * linp1..N form an ItemId array.  ItemPointers point into this array
 * rather than pointing directly to a tuple.  Note that OffsetNumbers
 * conventionally start at 1, not 0.
 *
 * tuple1..N are added "backwards" on the page.  because a tuple's
 * ItemPointer points to its ItemId entry rather than its actual
 * byte-offset position, tuples can be physically shuffled on a page
 * whenever the need arises.
 *
 * AM-generic per-page information is kept in PageHeaderData.
 *
 * AM-specific per-page data (if any) is kept in the area marked "special
 * space"; each AM has an "opaque" structure defined somewhere that is
 * stored as the page trailer.  an access method should always
 * initialize its pages with PageInit and then set its own opaque
 * fields.
 */
 
 其中，PageHeaderData包含内容如下：
 PageHeaderDataEle.pd_lsn.xlogid = 0
PageHeaderDataEle.pd_lsn.xrecoff = 315817760
PageHeaderDataEle.pd_checksum = 0
PageHeaderDataEle.pd_flags = 0
PageHeaderDataEle.pd_lower = 92
PageHeaderDataEle.pd_upper = 7552
PageHeaderDataEle.pd_special = 8192
PageHeaderDataEle.pd_pagesize_version = 8196
PageHeaderDataEle.pd_prune_xid = 1321
PageHeaderDataEle->pd_linp.lp_off = 8152
PageHeaderDataEle->pd_linp.lp_flags = 1
PageHeaderDataEle->pd_linp.lp_len = 40
PageHeaderDataEle->pd_linp.lp_off = 8112
PageHeaderDataEle->pd_linp.lp_flags = 1
PageHeaderDataEle->pd_linp.lp_len = 40
....
具体内容含义，可以阅读《PostgreSQL数据库内核分析》P58
linpx 是一个结构体，如下：
typedef struct ItemIdData
{
	unsigned	lp_off:15,		/* offset to tuple (from start of page) */
				lp_flags:2,		/* state of item pointer, see below */
				lp_len:15;		/* byte length of tuple */
} ItemIdData;
标记了对应的tuple在页中offset位置，该tuple状态，该tuple长度。
/*
 * lp_flags has these possible states.  An UNUSED line pointer is available
 * for immediate re-use, the other states are not.
 */
#define LP_UNUSED		0		/* unused (should always have lp_len=0) */
#define LP_NORMAL		1		/* used (should always have lp_len>0) */
#define LP_REDIRECT		2		/* HOT redirect (should have lp_len=0) */
#define LP_DEAD			3		/* dead, may or may not have storage */

现在我们已经已经知道了表数据文件内部数据结构，不考虑可见性判断mvcc，那么我们可以根据这些内容读出表中所存储的数据内容。
现实不会这么简单，比如需要考虑可见性判断、一个tuple中字段attr个数、是否含有空行、是否有oid、元组头长度（tuple长度除去实际表
数据长度）。


//see: src/include/storage/bufpage.h

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned char uint8;    /* == 8 bits */
typedef unsigned short uint16;  /* == 16 bits */
typedef unsigned int uint32;    /* == 32 bits */



//add by wln

#define FLEXIBLE_ARRAY_MEMBER 1

/*
 * For historical reasons, the 64-bit LSN value is stored as two 32-bit
 * values.
 */
typedef struct
{
        uint32          xlogid;                 /* high bits */
        uint32          xrecoff;                /* low bits */
} PageXLogRecPtr;

/*
 * location (byte offset) within a page.
 *
 * note that this is actually limited to 2^15 because we have limited
 * ItemIdData.lp_off and ItemIdData.lp_len to 15 bits (see itemid.h).
 */
typedef uint16 LocationIndex;


typedef uint32 TransactionId;

/*
 * An item pointer (also called line pointer) on a buffer page
 *
 * In some cases an item pointer is "in use" but does not have any associated
 * storage on the page.  By convention, lp_len == 0 in every item pointer
 * that does not have storage, independently of its lp_flags state.
 */
typedef struct ItemIdData //标示 tuple位置相关信息的结构体
{
        unsigned        lp_off:15,              /* offset to tuple (from start of page) */
                                lp_flags:2,             /* state of item pointer, see below */
                                lp_len:15;              /* byte length of tuple */
} ItemIdData;




typedef struct PageHeaderData
{
        /* XXX LSN is member of *any* block, not only page-organized ones */
        PageXLogRecPtr pd_lsn;          /* LSN: next byte after last byte of xlog
                                                                 * record for last change to this page */
        uint16          pd_checksum;    /* checksum */
        uint16          pd_flags;               /* flag bits, see below */
        LocationIndex pd_lower;         /* offset to start of free space */
        LocationIndex pd_upper;         /* offset to end of free space */
        LocationIndex pd_special;       /* offset to start of special space */
        uint16          pd_pagesize_version;
        TransactionId pd_prune_xid; /* oldest prunable XID, or zero if none */
        ItemIdData      pd_linp[FLEXIBLE_ARRAY_MEMBER]; /* line pointer array */
} PageHeaderData;

#define PageXLogRecPtrGet(val) \
        ((uint64) (val).xlogid << 32 | (val).xrecoff)
#define PageXLogRecPtrSet(ptr, lsn) \
        ((ptr).xlogid = (uint32) ((lsn) >> 32), (ptr).xrecoff = (uint32) (lsn))

/*
*  this macro used to print variable
*/
#define print(n)  printf(#n" = %d\n",n)

int main(int argc,char *argv[])
{
 if(argc<2 || (argc>=2 && access(argv[1], F_OK)))
  {
    printf("file:%s doesnot exist.\n",argv[1]); 
    exit(1);
  }

 char filename[1024];
 strncpy(filename,argv[1],strlen(argv[1])+1); //must +1
 printf("filename:%s,%d\n",filename,strlen(filename));
 int fd;

 if((fd=open(filename,O_RDONLY))==-1)
 {
    printf("%m,%s,line:%d\n",__FILE__,__LINE__);
    exit(1);
 }

 PageHeaderData  PageHeaderDataEle;//=(PageHeaderData *)malloc(sizeof(PageHeaderData)); 
 if(read(fd,&PageHeaderDataEle,sizeof(PageHeaderDataEle))==-1)
 {
    printf("%m,%s,line:%d\n",__FILE__,__LINE__);
    exit(1);
 }
  printf("sizeof(PageHeaderDataEle)=%d\n",sizeof(PageHeaderDataEle));

  close(fd); 

  print(PageHeaderDataEle.pd_lsn.xlogid);
  print(PageHeaderDataEle.pd_lsn.xrecoff);
  print(PageHeaderDataEle.pd_checksum);
  print(PageHeaderDataEle.pd_flags);  
  print(PageHeaderDataEle.pd_lower);
  print(PageHeaderDataEle.pd_upper);
  print(PageHeaderDataEle.pd_special);
  print(PageHeaderDataEle.pd_pagesize_version);
  print(PageHeaderDataEle.pd_prune_xid);
  ItemIdData *ItemIdDataEle = (ItemIdData *)malloc(sizeof(ItemIdData));
  memcpy(ItemIdDataEle,&(PageHeaderDataEle.pd_linp),sizeof(ItemIdData));
  printf("PageHeaderDataEle->pd_linp.lp_off = %d\n",ItemIdDataEle->lp_off);
  printf("PageHeaderDataEle->pd_linp.lp_flags = %d\n",ItemIdDataEle->lp_flags);
  printf("PageHeaderDataEle->pd_linp.lp_len = %d\n",ItemIdDataEle->lp_len);
   


 free(ItemIdDataEle);

}


-------------------------------------------------
test:
wln@iZ232ngsvp8Z:~/pg95> psql
psql (9.5beta1)
Type "help" for help.

postgres=# select pg_relation_filepath('c1');
 pg_relation_filepath 
----------------------
 base/12974/40997
(1 row)

postgres=# select * from page_header(get_raw_page('c1', 'main', 0));
    lsn     | checksum | flags | lower | upper | special | pagesize | version | prune_xid 
------------+----------+-------+-------+-------+---------+----------+---------+-----------
 0/12CC3B48 |        0 |     0 |   108 |  7520 |    8192 |     8192 |       4 |         0
(1 row)

postgres=# select lp,lp_off,lp_flags,lp_len from heap_page_items(get_raw_page('c1', 'main', 0));
 lp | lp_off | lp_flags | lp_len 
----+--------+----------+--------
  1 |   8160 |        1 |     26
  2 |   8128 |        1 |     26
  3 |   8096 |        1 |     26
  4 |   8064 |        1 |     26
  5 |   8032 |        1 |     26
  6 |   8000 |        1 |     26
  7 |   7968 |        1 |     26
  8 |   7936 |        1 |     26
  9 |   7904 |        1 |     26
 10 |   7872 |        1 |     26
 11 |   7840 |        1 |     26
 12 |   7808 |        1 |     26
 13 |   7776 |        1 |     26
 14 |   7744 |        1 |     26
 15 |   7712 |        1 |     26
 16 |   7680 |        1 |     26
 17 |   7648 |        1 |     26
 18 |   7616 |        1 |     26
 19 |   7584 |        1 |     26
 20 |   7552 |        1 |     26
 21 |   7520 |        1 |     26
(21 rows)

postgres=# \q
wln@iZ232ngsvp8Z:~/pg95> ./open data/base/12974/40997
filename:data/base/12974/40997,21
sizeof(PageHeaderDataEle)=28
PageHeaderDataEle.pd_lsn.xlogid = 0
PageHeaderDataEle.pd_lsn.xrecoff = 315374408
PageHeaderDataEle.pd_checksum = 0
PageHeaderDataEle.pd_flags = 0
PageHeaderDataEle.pd_lower = 108
PageHeaderDataEle.pd_upper = 7520
PageHeaderDataEle.pd_special = 8192
PageHeaderDataEle.pd_pagesize_version = 8196
PageHeaderDataEle.pd_prune_xid = 0
PageHeaderDataEle->pd_linp.lp_off = 8160
PageHeaderDataEle->pd_linp.lp_flags = 1
PageHeaderDataEle->pd_linp.lp_len = 26
wln@iZ232ngsvp8Z:~/pg95> hexdump data/base/12974/40997 -d
0000000   00000   00000   15176   04812   00000   00000   00108   07520
0000010   08192   08196   00000   00000   40928   00052   40896   00052
0000020   40864   00052   40832   00052   40800   00052   40768   00052
0000030   40736   00052   40704   00052   40672   00052   40640   00052
0000040   40608   00052   40576   00052   40544   00052   40512   00052
0000050   40480   00052   40448   00052   40416   00052   40384   00052
0000060   40352   00052   40320   00052   40288   00052   00000   00000
0000070   00000   00000   00000   00000   00000   00000   00000   00000
*
0001d60   01310   00000   00000   00000   00000   00000   00000   00000
0001d70   00021   00001   02050   00024   14597   00000   00000   00000
0001d80   01310   00000   00000   00000   00000   00000   00000   00000
0001d90   00020   00001   02050   00024   14341   00000   00000   00000
0001da0   01310   00000   00000   00000   00000   00000   00000   00000
0001db0   00019   00001   02050   00024   14085   00000   00000   00000
0001dc0   01310   00000   00000   00000   00000   00000   00000   00000
0001dd0   00018   00001   02050   00024   13829   00000   00000   00000
0001de0   01310   00000   00000   00000   00000   00000   00000   00000
0001df0   00017   00001   02050   00024   13573   00000   00000   00000
0001e00   01310   00000   00000   00000   00000   00000   00000   00000
0001e10   00016   00001   02050   00024   13317   00000   00000   00000
0001e20   01310   00000   00000   00000   00000   00000   00000   00000
0001e30   00015   00001   02050   00024   13061   00000   00000   00000
0001e40   01310   00000   00000   00000   00000   00000   00000   00000
0001e50   00014   00001   02050   00024   12805   00000   00000   00000
0001e60   01310   00000   00000   00000   00000   00000   00000   00000
0001e70   00013   00001   02050   00024   12549   00000   00000   00000
0001e80   01309   00000   00000   00000   00000   00000   00000   00000
0001e90   00012   00001   02050   00024   14597   00000   00000   00000
0001ea0   01309   00000   00000   00000   00000   00000   00000   00000
0001eb0   00011   00001   02050   00024   14341   00000   00000   00000
0001ec0   01309   00000   00000   00000   00000   00000   00000   00000
0001ed0   00010   00001   02050   00024   14085   00000   00000   00000
0001ee0   01309   00000   00000   00000   00000   00000   00000   00000
0001ef0   00009   00001   02050   00024   13829   00000   00000   00000
0001f00   01309   00000   00000   00000   00000   00000   00000   00000
0001f10   00008   00001   02050   00024   13573   00000   00000   00000
0001f20   01309   00000   00000   00000   00000   00000   00000   00000
0001f30   00007   00001   02050   00024   13317   00000   00000   00000
0001f40   01309   00000   00000   00000   00000   00000   00000   00000
0001f50   00006   00001   02050   00024   13061   00000   00000   00000
0001f60   01309   00000   00000   00000   00000   00000   00000   00000
0001f70   00005   00001   02050   00024   12805   00000   00000   00000
0001f80   01309   00000   00000   00000   00000   00000   00000   00000
0001f90   00004   00001   02050   00024   12549   00000   00000   00000
0001fa0   01307   00000   00000   00000   00000   00000   00000   00000
0001fb0   00003   00001   02050   00024   12549   00000   00000   00000
0001fc0   01306   00000   00000   00000   00000   00000   00000   00000
0001fd0   00002   00001   02050   00024   25093   00000   00000   00000
0001fe0   01305   00000   00000   00000   00000   00000   00000   00000
0001ff0   00001   00001   02050   00024   24837   00000   00000   00000
0002000
wln@iZ232ngsvp8Z:~/pg95> hexdump data/base/12974/40997 
0000000 0000 0000 3b48 12cc 0000 0000 006c 1d60
0000010 2000 2004 0000 0000 9fe0 0034 9fc0 0034
0000020 9fa0 0034 9f80 0034 9f60 0034 9f40 0034
0000030 9f20 0034 9f00 0034 9ee0 0034 9ec0 0034
0000040 9ea0 0034 9e80 0034 9e60 0034 9e40 0034
0000050 9e20 0034 9e00 0034 9de0 0034 9dc0 0034
0000060 9da0 0034 9d80 0034 9d60 0034 0000 0000
0000070 0000 0000 0000 0000 0000 0000 0000 0000
*
0001d60 051e 0000 0000 0000 0000 0000 0000 0000
0001d70 0015 0001 0802 0018 3905 0000 0000 0000
0001d80 051e 0000 0000 0000 0000 0000 0000 0000
0001d90 0014 0001 0802 0018 3805 0000 0000 0000
0001da0 051e 0000 0000 0000 0000 0000 0000 0000
0001db0 0013 0001 0802 0018 3705 0000 0000 0000
0001dc0 051e 0000 0000 0000 0000 0000 0000 0000
0001dd0 0012 0001 0802 0018 3605 0000 0000 0000
0001de0 051e 0000 0000 0000 0000 0000 0000 0000
0001df0 0011 0001 0802 0018 3505 0000 0000 0000
0001e00 051e 0000 0000 0000 0000 0000 0000 0000
0001e10 0010 0001 0802 0018 3405 0000 0000 0000
0001e20 051e 0000 0000 0000 0000 0000 0000 0000
0001e30 000f 0001 0802 0018 3305 0000 0000 0000
0001e40 051e 0000 0000 0000 0000 0000 0000 0000
0001e50 000e 0001 0802 0018 3205 0000 0000 0000
0001e60 051e 0000 0000 0000 0000 0000 0000 0000
0001e70 000d 0001 0802 0018 3105 0000 0000 0000
0001e80 051d 0000 0000 0000 0000 0000 0000 0000
0001e90 000c 0001 0802 0018 3905 0000 0000 0000
0001ea0 051d 0000 0000 0000 0000 0000 0000 0000
0001eb0 000b 0001 0802 0018 3805 0000 0000 0000
0001ec0 051d 0000 0000 0000 0000 0000 0000 0000
0001ed0 000a 0001 0802 0018 3705 0000 0000 0000
0001ee0 051d 0000 0000 0000 0000 0000 0000 0000
0001ef0 0009 0001 0802 0018 3605 0000 0000 0000
0001f00 051d 0000 0000 0000 0000 0000 0000 0000
0001f10 0008 0001 0802 0018 3505 0000 0000 0000
0001f20 051d 0000 0000 0000 0000 0000 0000 0000
0001f30 0007 0001 0802 0018 3405 0000 0000 0000
0001f40 051d 0000 0000 0000 0000 0000 0000 0000
0001f50 0006 0001 0802 0018 3305 0000 0000 0000
0001f60 051d 0000 0000 0000 0000 0000 0000 0000
0001f70 0005 0001 0802 0018 3205 0000 0000 0000
0001f80 051d 0000 0000 0000 0000 0000 0000 0000
0001f90 0004 0001 0802 0018 3105 0000 0000 0000
0001fa0 051b 0000 0000 0000 0000 0000 0000 0000
0001fb0 0003 0001 0802 0018 3105 0000 0000 0000
0001fc0 051a 0000 0000 0000 0000 0000 0000 0000
0001fd0 0002 0001 0802 0018 6205 0000 0000 0000
0001fe0 0519 0000 0000 0000 0000 0000 0000 0000
0001ff0 0001 0001 0802 0018 6105 0000 0000 0000
0002000

有关PageHeaderDataEle.pd_lsn.xrecoff：
wln@iZ232ngsvp8Z:~> printf "%d\n" 0x12cc3b48
315374408

有关pd_pagesize_version
/*
 * PageGetPageLayoutVersion
 *		Returns the page layout version of a page.
 */
#define PageGetPageLayoutVersion(page) \
	(((PageHeader) (page))->pd_pagesize_version & 0x00FF)
wln@iZ232ngsvp8Z:~/pg95> printf "%x\n" 8196
2004
而0x2004 &0x00ff 则对应4

有关pagesize
/*
 * PageGetPageSize
 *		Returns the page size of a page.
 *
 * this can only be called on a formatted page (unlike
 * BufferGetPageSize, which can be called on an unformatted page).
 * however, it can be called on a page that is not stored in a buffer.
 */
#define PageGetPageSize(page) \
	((Size) (((PageHeader) (page))->pd_pagesize_version & (uint16) 0xFF00))
则0x2004 & 0xff00=0x2000为8192


可以看出，open函数读出的内容，和hexdump及函数读出的内容完全一致。

有关标示tuple位置相关信息的结构体ItemIdData	pd_linp[FLEXIBLE_ARRAY_MEMBER]
在上面代码free(ItemIdDataEle)上面增加如下（前提是对应的表中至少插入了11行数据）：
  ItemIdData ItemIdDataArray[10];
  if(read(fd,&ItemIdDataArray,sizeof(ItemIdDataArray)*10)==-1)
  {
      printf("%m,%s,line:%d\n",__FILE__,__LINE__);
      exit(1);
  }
  int i=0;
  while(i<10)
  {
     printf("PageHeaderDataEle->pd_linp.lp_off = %d\n",ItemIdDataArray[i].lp_off);
     printf("PageHeaderDataEle->pd_linp.lp_flags = %d\n",ItemIdDataArray[i].lp_flags);
     printf("PageHeaderDataEle->pd_linp.lp_len = %d\n",ItemIdDataArray[i].lp_len);
     i++;
  }

 执行如下：
wln@iZ232ngsvp8Z:~/pg95> ./open data/base/12974/40997
filename:data/base/12974/40997,21
sizeof(PageHeaderDataEle)=28
PageHeaderDataEle.pd_lsn.xlogid = 0
PageHeaderDataEle.pd_lsn.xrecoff = 315509568
PageHeaderDataEle.pd_checksum = 0
PageHeaderDataEle.pd_flags = 0
PageHeaderDataEle.pd_lower = 112
PageHeaderDataEle.pd_upper = 7488
PageHeaderDataEle.pd_special = 8192
PageHeaderDataEle.pd_pagesize_version = 8196
PageHeaderDataEle.pd_prune_xid = 0
PageHeaderDataEle->pd_linp.lp_off = 8160
PageHeaderDataEle->pd_linp.lp_flags = 1
PageHeaderDataEle->pd_linp.lp_len = 26
PageHeaderDataEle->pd_linp.lp_off = 8128
PageHeaderDataEle->pd_linp.lp_flags = 1
PageHeaderDataEle->pd_linp.lp_len = 26
PageHeaderDataEle->pd_linp.lp_off = 8096
PageHeaderDataEle->pd_linp.lp_flags = 1
PageHeaderDataEle->pd_linp.lp_len = 26
PageHeaderDataEle->pd_linp.lp_off = 8064
PageHeaderDataEle->pd_linp.lp_flags = 1
PageHeaderDataEle->pd_linp.lp_len = 26
PageHeaderDataEle->pd_linp.lp_off = 8032
PageHeaderDataEle->pd_linp.lp_flags = 1
PageHeaderDataEle->pd_linp.lp_len = 26
PageHeaderDataEle->pd_linp.lp_off = 8000
PageHeaderDataEle->pd_linp.lp_flags = 1
PageHeaderDataEle->pd_linp.lp_len = 26
PageHeaderDataEle->pd_linp.lp_off = 7968
PageHeaderDataEle->pd_linp.lp_flags = 1
PageHeaderDataEle->pd_linp.lp_len = 26
PageHeaderDataEle->pd_linp.lp_off = 7936
PageHeaderDataEle->pd_linp.lp_flags = 1
PageHeaderDataEle->pd_linp.lp_len = 26
PageHeaderDataEle->pd_linp.lp_off = 7904
PageHeaderDataEle->pd_linp.lp_flags = 1
PageHeaderDataEle->pd_linp.lp_len = 26
PageHeaderDataEle->pd_linp.lp_off = 7872
PageHeaderDataEle->pd_linp.lp_flags = 1
PageHeaderDataEle->pd_linp.lp_len = 26
PageHeaderDataEle->pd_linp.lp_off = 7840
PageHeaderDataEle->pd_linp.lp_flags = 1
PageHeaderDataEle->pd_linp.lp_len = 26
wln@iZ232ngsvp8Z:~/pg95> psql
psql (9.5beta1)
Type "help" for help.

postgres=# select lp,lp_off,lp_flags,lp_len from heap_page_items(get_raw_page('c1', 'main', 0));
 lp | lp_off | lp_flags | lp_len 
----+--------+----------+--------
  1 |   8160 |        1 |     26
  2 |   8128 |        1 |     26
  3 |   8096 |        1 |     26
  4 |   8064 |        1 |     26
  5 |   8032 |        1 |     26
  6 |   8000 |        1 |     26
  7 |   7968 |        1 |     26
  8 |   7936 |        1 |     26
  9 |   7904 |        1 |     26
 10 |   7872 |        1 |     26
 11 |   7840 |        1 |     26
 12 |   7808 |        1 |     26
 13 |   7776 |        1 |     26
 14 |   7744 |        1 |     26
 15 |   7712 |        1 |     26
 16 |   7680 |        1 |     26
 17 |   7648 |        1 |     26
 18 |   7616 |        1 |     26
 19 |   7584 |        1 |     26
 20 |   7552 |        1 |     26
 21 |   7520 |        1 |     26
 22 |   7488 |        1 |     26
(22 rows)
 


参考：http://my.oschina.net/Suregogo/blog/595335?fromerr=4Ru4VQhW
