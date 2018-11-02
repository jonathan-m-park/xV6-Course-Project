#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char **argv)
{
  if(3 != argc){
    printf(1, "Error: chgrp usage: chgrp gid path!\n");
    exit();
  }

  if(0 > chgrp(argv[2], atoi(argv[1]))){
    printf(1, "Error: chgrp failed.\n");
    exit();
  }

  exit();
}

#endif
