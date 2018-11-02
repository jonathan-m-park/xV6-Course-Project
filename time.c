#ifdef CS333_P2
#include "types.h"
#include "user.h"

static void printtime (uint time);
static int Fork(void);
static void Exec(char*, char**);

int
main(int argc, char *argv[])
{
  int rc;
  char *user_function = argv[1];
  char **user_arguments = argv + 1;
  uint start_ticks = 0;
  uint finish_ticks;
  
  if (argc < 2){
    start_ticks = uptime ();  //return time of time
    finish_ticks = uptime () - start_ticks;
    printf(2, " ran in ");
    printtime(finish_ticks);
    exit ();
    }
  else{
    start_ticks = uptime();
    rc = Fork();
    if (0 == rc){
      Exec(user_function, user_arguments);
    }
    wait ();
    finish_ticks = uptime() - start_ticks;
    printf(2, "%s ran in ", user_function);
    printtime(finish_ticks);
    exit();
  }
  
}

/* Converts the ticks vales to seconds. */
/* Ticks are milisecs so will use 1000 as converison rate */
/* The prints display accounting for 0s */

static void
printtime (uint time)
{
  uint q, r;
  q = time / 1000;
  r = time % 1000;
  if (100 > r && 9 < r)
    printf(2, "%d.0%d seconds\n", q, r);
  else if (10 > r)
    printf(2, "%d.00%d seconds\n", q, r);
  else
    printf(2, "%d.%d seconds\n", q, r);
}

static int
Fork(void)
{
    int rc = fork();
    if (0 > rc){
      printf(2, "Error: fork call failed. %s at line %d\n",
	     __FILE__, __LINE__);
      exit();
    }
    return rc;
}

static void
Exec(char *user_function, char **user_arguments)
{
  int rc = exec (user_function, user_arguments);
  if (0 > rc){
    printf(2, "Error: exec %s failed.\n", user_function);
    exit();
  }
}
      
#endif
