#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char **argv)
{
  if(3 != argc){
    printf(1, "Error: chmod usage: chmod mode path!\n");
    exit();
  }

  if(0 > chmod(argv[2], atog(argv[1], 8))){
    printf(1, "Error: chmod failed.\n");
    exit();
  }

  exit();
}

#endif
