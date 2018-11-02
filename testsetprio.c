/* The following is a test program to verify setpriority for sleeping processes
as well as erroraneous arguments for set priority */

#include "types.h"
#include "user.h"

#define NPRIO 3
#define NUM_CHILDREN 1

int
main(void)
{
  int i, rc;
  for (i = 0; i < NUM_CHILDREN; i++){
    rc = fork();
    if (!rc)// child
      sleep(1000);
  }
  if(rc){
    sleep(1000);
    printf(1, "Setting PID(%d) to be 1\n", getpid());
    if(0 == setpriority(getpid(), 1))
      printf(1, "Success!!\n");
    else{
      printf(1, "ERROR: setpriority returned -1\n");
      exit();
    }
    printf(1, "Setting PID(2) to be 1\n", getpid());
    if(0 == setpriority(2, 1))
      printf(1, "Success!!\n");
    else{
      printf(1, "ERROR: setpriority returned -1\n");
      exit();
    }
    sleep(1000);
    printf(1, "Setting PID(%d) to be MAX\n", getpid());
    if(0 == setpriority(getpid(), NPRIO - 1))
      printf(1, "Success!!\n");
    else{
      printf(1, "ERROR: setpriority returned -1\n");
      exit();
    }
    sleep(1000);
    printf(1, "Setting PID(%d) to be an invalid priority\n", getpid());
    if(0 > setpriority(getpid(), NPRIO + 1))
      printf(1, "setpriority call failed as expected.\n");
    else{
      printf(1, "ERROR: setpriority did not return -1\n");
      exit();
    }
    printf(1, "Calling setpriority with an invalid PID(%d)\n", getpid() + 70);
    if(0 > setpriority(getpid()  +70, 0))
      printf(1, "setpriority call failed as expected.\n");
    else{
      printf(1, "ERROR: setpriority did not return -1\n");
      exit();
    } 
  }
}
