#include "types.h"
#include "syscall.h"
#include "user.h"

// thanks to taniya for the following test program idea!!!!
int
main(void)
{
  int pid[10], ppid = getpid(), i, x;

  for(i = 0; i < 10; i++){
    pid[i] = fork();
    if(0 > pid[i])
      break;
    if(getpid() != ppid)
    {
      /* sleep(6000); */
      wait();
      exit();
    }
  }
  if(getpid() == ppid)
    {
      printf(1, "lsag\n");
      sleep(1000);
      for(x = 0; x < 64; x++){
	kill(pid[x]);
	sleep(1000);
	while(0 > wait())
	  ;
	printf(1, "Reaped PID:%d\n", pid[x]);
      }
    }
  exit();
}
