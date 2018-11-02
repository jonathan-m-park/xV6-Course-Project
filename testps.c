#include "types.h"
#include "user.h"
#include "syscall.h"

int main (void)
{
  int rc;
  uint start_ticks = uptime();
  rc = fork();
  if (0 > rc){
    printf(2, "ERROR: fork failed.\n");
    exit();
  }
  if (0 == rc){
    rc = fork();
    if (0 > rc){
      printf(2, "ERROR: fork failed.\n");
      exit();
    }
    else{
      while(6000 > uptime() - start_ticks)
        ;
    }
  }
  wait();
  exit();
 }
