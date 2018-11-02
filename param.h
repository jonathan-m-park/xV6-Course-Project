#define NPROC                64  // maximum number of processes
#define KSTACKSIZE         4096  // size of per-process kernel stack
#define NCPU                  8  // maximum number of CPUs
#define NOFILE               16  // open files per process
#define NFILE               100 // open files per system
#define NINODE               50  // maximum number of active i-nodes
#define NDEV                 10  // maximum major device number
#define ROOTDEV               1  // device number of file system root disk
#define MAXARG               32  // max exec arguments
#define MAXOPBLOCKS          10  // max # of blocks any FS op writes
#define LOGSIZE (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF    (MAXOPBLOCKS*3)  // size of disk block cache
// #define FSSIZE          1000  // size of file system in blocks
#define FSSIZE             2000  // size of file system in blocks  // CS333 requires a larger FS.

#ifdef CS333_P2
#define DEFAULTUID            0  // Default user ID value for both the first process and files created by mkfs
                                 // during the creation of the file system
#define DEFAULTGID            0  // Default group ID for both the first process and files created by mkfs
                                 // during the creation of the file system
#endif
#ifdef CS333_P5
#define DEFAULTMODE       0755  // Default mode interpreted as no setuid bit, full owner permissions,
                                 // group permissions to read and execute, and other permissions to read and execute.
                                 // This is the default file permission.
#endif

#ifdef CS333_P3P4
#define MAX                   6  // maximum priority value for MLFQ
#define TICKS_TO_PROMOTE    500  // tick interval to promote
#define BUDGET             1700  // time budget for demotion
#endif
