#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* this file has the same effect as the binary file 'pg_contrldata' in postgres, haha,the file's content comes from Postgres.
 * gcc -o aim sourcefile; ./aim (put the aim file in PGDATA path).
 * I use this file to get understand the xlog.
 * 2015.03.08 linwanggm@gmail.com
 */

typedef long long  int uint64;
typedef  int uint32;

int fd;

/*
 *  * System status indicator.  Note this is stored in pg_control; if you change
 *   * it, you must bump PG_CONTROL_VERSION
 *    */
typedef enum DBState
{
	DB_STARTUP = 0,
	DB_SHUTDOWNED,
	DB_SHUTDOWNED_IN_RECOVERY,
	DB_SHUTDOWNING,
	DB_IN_CRASH_RECOVERY,
	DB_IN_ARCHIVE_RECOVERY,
	DB_IN_PRODUCTION
} DBState;

typedef uint64 pg_time_t;

/*
 *  * Pointer to a location in the XLOG.  These pointers are 64 bits wide,
 *   * because we don't want them ever to overflow.
 *    */
typedef uint64 XLogRecPtr;

typedef uint32 TimeLineID;
typedef uint32 TransactionId;

typedef unsigned int Oid;
/* MultiXactId must be equivalent to TransactionId, to fit in t_xmax */
typedef TransactionId MultiXactId;

typedef uint32 MultiXactOffset;


typedef uint32 pg_crc32;
typedef uint bool;
/*
 *  * Body of CheckPoint XLOG records.  This is declared here because we keep
 *   * a copy of the latest one in pg_control for possible disaster recovery.
 *    * Changing this struct requires a PG_CONTROL_VERSION bump.
 *     */
typedef struct CheckPoint
{
	XLogRecPtr	redo;			/* next RecPtr available when we began to
								 * create CheckPoint (i.e. REDO start point) */
	TimeLineID	ThisTimeLineID; /* current TLI */
	TimeLineID	PrevTimeLineID; /* previous TLI, if this record begins a new
								 * timeline (equals ThisTimeLineID otherwise) */
	bool		fullPageWrites; /* current full_page_writes */
	uint32		nextXidEpoch;	/* higher-order bits of nextXid */
	TransactionId nextXid;		/* next free XID */
	Oid			nextOid;		/* next free OID */
	MultiXactId nextMulti;		/* next free MultiXactId */
	MultiXactOffset nextMultiOffset;	/* next free MultiXact offset */
	TransactionId oldestXid;	/* cluster-wide minimum datfrozenxid */
	Oid			oldestXidDB;	/* database with minimum datfrozenxid */
	MultiXactId oldestMulti;	/* cluster-wide minimum datminmxid */
	Oid			oldestMultiDB;	/* database with minimum datminmxid */
	pg_time_t	time;			/* time stamp of checkpoint */

	/*
 * 	 * Oldest XID still running. This is only needed to initialize hot standby
 * 	 	 * mode from an online checkpoint, so we only bother calculating this for
 * 	 	 	 * online checkpoints and only when wal_level is hot_standby. Otherwise
 * 	 	 	 	 * it's set to InvalidTransactionId.
 * 	 	 	 	 	 */
	TransactionId oldestActiveXid;
} CheckPoint;

typedef struct ControlFileData
{
	/*
	 * Unique system identifier --- to ensure we match up xlog files with the
	 * installation that produced them.
	 */
	uint64		system_identifier;

	/*
	 * Version identifier information.  Keep these fields at the same offset,
	 * especially pg_control_version; they won't be real useful if they move
	 * around.  (For historical reasons they must be 8 bytes into the file
	 * rather than immediately at the front.)
	 *
	 * pg_control_version identifies the format of pg_control itself.
	 * catalog_version_no identifies the format of the system catalogs.
	 *
	 * There are additional version identifiers in individual files; for
	 * example, WAL logs contain per-page magic numbers that can serve as
	 * version cues for the WAL log.
	 */
	uint32		pg_control_version;		/* PG_CONTROL_VERSION */
	uint32		catalog_version_no;		/* see catversion.h */

	/*
	 * System status data
	 */
	DBState		state;			/* see enum above */
	pg_time_t	time;			/* time stamp of last pg_control update */
	XLogRecPtr	checkPoint;		/* last check point record ptr */
	XLogRecPtr	prevCheckPoint; /* previous check point record ptr */

	CheckPoint	checkPointCopy; /* copy of last check point record */

	XLogRecPtr	unloggedLSN;	/* current fake LSN value, for unlogged rels */

	/*
	 * These two values determine the minimum point we must recover up to
	 * before starting up:
	 *
	 * minRecoveryPoint is updated to the latest replayed LSN whenever we
	 * flush a data change during archive recovery. That guards against
	 * starting archive recovery, aborting it, and restarting with an earlier
	 * stop location. If we've already flushed data changes from WAL record X
	 * to disk, we mustn't start up until we reach X again. Zero when not
	 * doing archive recovery.
	 *
	 * backupStartPoint is the redo pointer of the backup start checkpoint, if
	 * we are recovering from an online backup and haven't reached the end of
	 * backup yet. It is reset to zero when the end of backup is reached, and
	 * we mustn't start up before that. A boolean would suffice otherwise, but
	 * we use the redo pointer as a cross-check when we see an end-of-backup
	 * record, to make sure the end-of-backup record corresponds the base
	 * backup we're recovering from.
	 *
	 * backupEndPoint is the backup end location, if we are recovering from an
	 * online backup which was taken from the standby and haven't reached the
	 * end of backup yet. It is initialized to the minimum recovery point in
	 * pg_control which was backed up last. It is reset to zero when the end
	 * of backup is reached, and we mustn't start up before that.
	 *
	 * If backupEndRequired is true, we know for sure that we're restoring
	 * from a backup, and must see a backup-end record before we can safely
	 * start up. If it's false, but backupStartPoint is set, a backup_label
	 * file was found at startup but it may have been a leftover from a stray
	 * pg_start_backup() call, not accompanied by pg_stop_backup().
	 */
	XLogRecPtr	minRecoveryPoint;
	TimeLineID	minRecoveryPointTLI;
	XLogRecPtr	backupStartPoint;
	XLogRecPtr	backupEndPoint;
	bool		backupEndRequired;

	/*
	 * Parameter settings that determine if the WAL can be used for archival
	 * or hot standby.
	 */
	int			wal_level;
	bool		wal_log_hints;
	int			MaxConnections;
	int			max_worker_processes;
	int			max_prepared_xacts;
	int			max_locks_per_xact;

	/*
	 * This data is used to check for hardware-architecture compatibility of
	 * the database and the backend executable.  We need not check endianness
	 * explicitly, since the pg_control version will surely look wrong to a
	 * machine of different endianness, but we do need to worry about MAXALIGN
	 * and floating-point format.  (Note: storage layout nominally also
	 * depends on SHORTALIGN and INTALIGN, but in practice these are the same
	 * on all architectures of interest.)
	 *
	 * Testing just one double value is not a very bulletproof test for
	 * floating-point compatibility, but it will catch most cases.
	 */
	uint32		maxAlign;		/* alignment requirement for tuples */
	double		floatFormat;	/* constant 1234567.0 */
#define FLOATFORMAT_VALUE	1234567.0

	/*
	 * This data is used to make sure that configuration of this database is
	 * compatible with the backend executable.
	 */
	uint32		blcksz;			/* data block size for this DB */
	uint32		relseg_size;	/* blocks per segment of large relation */

	uint32		xlog_blcksz;	/* block size within WAL files */
	uint32		xlog_seg_size;	/* size of each WAL segment */

	uint32		nameDataLen;	/* catalog name field width */
	uint32		indexMaxKeys;	/* max number of columns in an index */

	uint32		toast_max_chunk_size;	/* chunk size in TOAST tables */
	uint32		loblksize;		/* chunk size in pg_largeobject */

	/* flag indicating internal format of timestamp, interval, time */
	bool		enableIntTimes; /* int64 storage enabled? */

	/* flags indicating pass-by-value status of various types */
	bool		float4ByVal;	/* float4 pass-by-value? */
	bool		float8ByVal;	/* float8, int8, etc pass-by-value? */

	/* Are data pages protected by checksums? Zero if no checksum version */
	uint32		data_checksum_version;

	/* CRC of all above ... MUST BE LAST! */
	pg_crc32	crc;
} ControlFileData;


static const char *
dbState(DBState state)
{
	switch (state)
	{
		case DB_STARTUP:
			return ("starting up");
		case DB_SHUTDOWNED:
			return ("shut down");
		case DB_SHUTDOWNED_IN_RECOVERY:
			return ("shut down in recovery");
		case DB_SHUTDOWNING:
			return ("shutting down");
		case DB_IN_CRASH_RECOVERY:
			return ("in crash recovery");
		case DB_IN_ARCHIVE_RECOVERY:
			return ("in archive recovery");
		case DB_IN_PRODUCTION:
			return ("in production");
	}
	return ("unrecognized status code");
}

/* WAL levels */
typedef enum WalLevel
{
	WAL_LEVEL_MINIMAL = 0,
	WAL_LEVEL_ARCHIVE,
	WAL_LEVEL_HOT_STANDBY,
	WAL_LEVEL_LOGICAL
} WalLevel;
extern int	wal_level;

static const char *
wal_level_str(WalLevel wal_level)
{
	switch (wal_level)
	{
		case WAL_LEVEL_MINIMAL:
			return "minimal";
		case WAL_LEVEL_ARCHIVE:
			return "archive";
		case WAL_LEVEL_HOT_STANDBY:
			return "hot_standby";
		case WAL_LEVEL_LOGICAL:
			return "logical";
	}
	return ("unrecognized wal_level");
}

int main()

{

ControlFileData ControlFile;

#define MAXPGPATH 1024
char		ControlFilePath[MAXPGPATH];

char *DataDir="/home/wln/pg/data";
char *progname="pg_controldata";
snprintf(ControlFilePath, MAXPGPATH, "%s/global/pg_control", DataDir);

time_t		time_tmp;


	if ((fd = open(ControlFilePath, O_RDONLY, 0)) == -1)
	{
		fprintf(stderr, ("%s: could not open file \"%s\" for reading: %s\n"),
				progname, ControlFilePath, strerror(errno));
		exit(2);
	}

	if (read(fd, &ControlFile, sizeof(ControlFileData)) != sizeof(ControlFileData))
	{
		fprintf(stderr, ("%s: could not read file \"%s\": %s\n"),
				progname, ControlFilePath, strerror(errno));
		exit(2);
	}
	close(fd);


	/*
 * 	 * This slightly-chintzy coding will work as long as the control file
 *  timestamps are within the range of time_t; that should be the case in
 *  all foreseeable circumstances, so we don't bother importing the
 * 	 backend's timezone library into pg_controldata.
 *   Use variable for format to suppress overly-anal-retentive gcc warning about %c
*/

/*
 *  * Compute ID and segment from an XLogRecPtr.
 *  For XLByteToSeg, do the computation at face value.  For XLByteToPrevSeg,
 * a boundary byte is taken to be in the previous segment.  This is suitable
 *  for deciding which segment to write given a pointer to a record end,for example.
 */

#define XLByteToSeg(xlrp, logSegNo) \
	logSegNo = (xlrp) / XLogSegSize

#define XLByteToPrevSeg(xlrp, logSegNo) \
	logSegNo = ((xlrp) - 1) / XLogSegSize


/*
 * 	 * Calculate name of the WAL file containing the latest checkpoint's REDO
 * 	 	 * start point.
 * 	 	 	 */
/*
 *  * The XLOG is split into WAL segments (physical files) of the size indicated
 *   * by XLOG_SEG_SIZE.
 *    */

#define XLOG_SEG_SIZE (16 * 1024 * 1024)
#define XLogSegSize		((uint32) XLOG_SEG_SIZE)
typedef uint64 XLogSegNo;
#define MAXFNAMELEN		64
char		xlogfilename[MAXFNAMELEN];
XLogSegNo	segno;
	XLByteToSeg(ControlFile.checkPointCopy.redo, segno);
#define XLogFileName(fname, tli, logSegNo)	\
	snprintf(fname, MAXFNAMELEN, "%08X%08X%08X", tli,		\
			 (uint32) ((logSegNo) / XLogSegmentsPerXLogId), \
			 (uint32) ((logSegNo) % XLogSegmentsPerXLogId))

#define UINT64CONST(x) ((uint64) x)

#define XLogSegmentsPerXLogId	(UINT64CONST(0x100000000) / XLOG_SEG_SIZE)

XLogFileName(xlogfilename, ControlFile.checkPointCopy.ThisTimeLineID, segno);
#define UINT64_FORMAT "%llu"
char            sysident_str[32];
snprintf(sysident_str, sizeof(sysident_str), UINT64_FORMAT,
                         ControlFile.system_identifier);
//----------------------------------------------------------------------------------------
    printf(("Catalog version number:               %u\n"),
                   ControlFile.catalog_version_no);
    printf(("Database system identifier:           %s\n"),
                   sysident_str);
    printf(("pg_control version number:            %u\n"),
                   ControlFile.pg_control_version);
	printf(("pg_control version number:            %u\n"),
		   ControlFile.pg_control_version);
	printf(("Database cluster state:               %s\n"),
		   dbState(ControlFile.state));
	//printf(("pg_control last modified:             %s\n"),
	//	   pgctime_str);
	printf(("Latest checkpoint location:           %X/%X\n"),
		   (uint32) (ControlFile.checkPoint >> 32),
		   (uint32) ControlFile.checkPoint);
	printf(("Prior checkpoint location:            %X/%X\n"),
		   (uint32) (ControlFile.prevCheckPoint >> 32),
		   (uint32) ControlFile.prevCheckPoint);
	printf(("Latest checkpoint's REDO location:    %X/%X\n"),
		   (uint32) (ControlFile.checkPointCopy.redo >> 32),
		   (uint32) ControlFile.checkPointCopy.redo);
	printf(("Latest checkpoint's REDO WAL file:    %s\n"),
		   xlogfilename);

 	printf(("Latest checkpoint's TimeLineID:       %u\n"),
		   ControlFile.checkPointCopy.ThisTimeLineID);
	printf(("Latest checkpoint's PrevTimeLineID:   %u\n"),
		   ControlFile.checkPointCopy.PrevTimeLineID);
	printf(("Latest checkpoint's full_page_writes: %s\n"),
		   ControlFile.checkPointCopy.fullPageWrites ? ("on") : ("off"));
	printf(("Latest checkpoint's NextXID:          %u/%u\n"),
		   ControlFile.checkPointCopy.nextXidEpoch,
		   ControlFile.checkPointCopy.nextXid);
	printf(("Latest checkpoint's NextOID:          %u\n"),
		   ControlFile.checkPointCopy.nextOid);
	printf(("Latest checkpoint's NextMultiXactId:  %u\n"),
		   ControlFile.checkPointCopy.nextMulti);
	printf(("Latest checkpoint's NextMultiOffset:  %u\n"),
		   ControlFile.checkPointCopy.nextMultiOffset);
	printf(("Latest checkpoint's oldestXID:        %u\n"),
		   ControlFile.checkPointCopy.oldestXid);
	printf(("Latest checkpoint's oldestXID's DB:   %u\n"),
		   ControlFile.checkPointCopy.oldestXidDB);
	printf(("Latest checkpoint's oldestActiveXID:  %u\n"),
		   ControlFile.checkPointCopy.oldestActiveXid);
	printf(("Latest checkpoint's oldestMultiXid:   %u\n"),
		   ControlFile.checkPointCopy.oldestMulti);
	printf(("Latest checkpoint's oldestMulti's DB: %u\n"),
		   ControlFile.checkPointCopy.oldestMultiDB);
//	printf(("Time of latest checkpoint:            %s\n"),
//		   ckpttime_str);
	printf(("Fake LSN counter for unlogged rels:   %X/%X\n"),
		   (uint32) (ControlFile.unloggedLSN >> 32),
		   (uint32) ControlFile.unloggedLSN);
	printf(("Minimum recovery ending location:     %X/%X\n"),
		   (uint32) (ControlFile.minRecoveryPoint >> 32),
		   (uint32) ControlFile.minRecoveryPoint);
	printf(("Min recovery ending loc's timeline:   %u\n"),
		   ControlFile.minRecoveryPointTLI);
	printf(("Backup start location:                %X/%X\n"),
		   (uint32) (ControlFile.backupStartPoint >> 32),
		   (uint32) ControlFile.backupStartPoint);
	printf(("Backup end location:                  %X/%X\n"),
		   (uint32) (ControlFile.backupEndPoint >> 32),
		   (uint32) ControlFile.backupEndPoint);
	printf(("End-of-backup record required:        %s\n"),
		   ControlFile.backupEndRequired ? ("yes") : ("no"));
	printf(("Current wal_level setting:            %s\n"),
		   wal_level_str(ControlFile.wal_level));
	printf(("Current wal_log_hints setting:        %s\n"),
		   ControlFile.wal_log_hints ? ("on") : ("off"));
	printf(("Current max_connections setting:      %d\n"),
		   ControlFile.MaxConnections);
	printf(("Current max_worker_processes setting: %d\n"),
		   ControlFile.max_worker_processes);
	printf(("Current max_prepared_xacts setting:   %d\n"),
		   ControlFile.max_prepared_xacts);
	printf(("Current max_locks_per_xact setting:   %d\n"),
		   ControlFile.max_locks_per_xact);
	printf(("Maximum data alignment:               %u\n"),
		   ControlFile.maxAlign);
//	// we don't print floatFormat since can't say much useful about it 
	printf(("Database block size:                  %u\n"),
		   ControlFile.blcksz);
	printf(("Blocks per segment of large relation: %u\n"),
		   ControlFile.relseg_size);
	printf(("WAL block size:                       %u\n"),
		   ControlFile.xlog_blcksz);
	printf(("Bytes per WAL segment:                %u\n"),
		   ControlFile.xlog_seg_size);
	printf(("Maximum length of identifiers:        %u\n"),
		   ControlFile.nameDataLen);
	printf(("Maximum columns in an index:          %u\n"),
		   ControlFile.indexMaxKeys);
	printf(("Maximum size of a TOAST chunk:        %u\n"),
		   ControlFile.toast_max_chunk_size);
	printf(("Size of a large-object chunk:         %u\n"),
		   ControlFile.loblksize);
	printf(("Date/time type storage:               %s\n"),
		   (ControlFile.enableIntTimes ? ("64-bit integers") : ("floating-point numbers")));
	printf(("Float4 argument passing:              %s\n"),
		   (ControlFile.float4ByVal ? ("by value") : ("by reference")));
	printf(("Float8 argument passing:              %s\n"),
		   (ControlFile.float8ByVal ? ("by value") : ("by reference")));
	printf(("Data page checksum version:           %u\n"),
		   ControlFile.data_checksum_version);
}

/*
*----------------------------------------------------------
*/
[wln@localhost data]$ ./r1
Catalog version number:               201306121
Database system identifier:           6087136306838363069
pg_control version number:            937
pg_control version number:            937
Database cluster state:               in production
Latest checkpoint location:           3/201E534
Prior checkpoint location:            3/201E4A0
Latest checkpoint's REDO location:    3/201E500
Latest checkpoint's REDO WAL file:    000000010000000300000002
Latest checkpoint's TimeLineID:       1
Latest checkpoint's PrevTimeLineID:   1
Latest checkpoint's full_page_writes: on
Latest checkpoint's NextXID:          0/1863
Latest checkpoint's NextOID:          24642
Latest checkpoint's NextMultiXactId:  1
Latest checkpoint's NextMultiOffset:  0
Latest checkpoint's oldestXID:        1768
Latest checkpoint's oldestXID's DB:   1
Latest checkpoint's oldestActiveXID:  1863
Latest checkpoint's oldestMultiXid:   1
Latest checkpoint's oldestMulti's DB: 1
Fake LSN counter for unlogged rels:   0/1
Minimum recovery ending location:     0/0
Min recovery ending loc's timeline:   0
Backup start location:                0/0
Backup end location:                  0/0
End-of-backup record required:        no
Current wal_level setting:            hot_standby
Current wal_log_hints setting:        on
Current max_connections setting:      0
Current max_worker_processes setting: 64
Current max_prepared_xacts setting:   4
Current max_locks_per_xact setting:   0
Maximum data alignment:               1093850759
Database block size:                  8192
Blocks per segment of large relation: 16777216
WAL block size:                       64
Bytes per WAL segment:                32
Maximum length of identifiers:        2000
Maximum columns in an index:          257
Maximum size of a TOAST chunk:        0
Size of a large-object chunk:         1126337551
Date/time type storage:               floating-point numbers
Float4 argument passing:              by reference
Float8 argument passing:              by reference
Data page checksum version:           0
