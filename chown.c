#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char **argv)
{
  if(3 != argc){
    printf(1, "Error: chown usage: chown uid path!\n");
    exit();
  }

  if(0 > chown(argv[2], atoi(argv[1]))){
    printf(1, "Error: chown failed.\n");
    exit();
  }

  exit();
}

#endif
