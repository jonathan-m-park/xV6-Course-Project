#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device

#ifdef CS333_P5
// mode union copied to avoid compiler directives and to maintain the coding style of xv6
union stat_mode_t {
  struct {
    // "other" bits 
    uint o_x : 1; // execute permission
    uint o_w : 1; // write permission
    uint o_r : 1; // read permission
    // "group" bits
    uint g_x : 1;
    uint g_w : 1;
    uint g_r : 1;
    // "user" bits
    uint u_x : 1;
    uint u_w : 1;
    uint u_r : 1;
    // setuid bit
    uint setuid : 1;
    // Padding for 32 bit int
    uint     : 22;
  } flags;
  uint asInt;
};
#endif

struct stat {
  short type;  // Type of file
  int dev;     // File system's disk device
  uint ino;    // Inode number
  short nlink; // Number of links to file
  uint size;   // Size of file in bytes
  #ifdef CS333_P5
  ushort uid;  // user ID for file
  ushort gid;  // group ID for file 
  union stat_mode_t mode; // file mode protection bits
  #endif
};
