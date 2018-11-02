#ifdef CS333_P2
#include "types.h"
#include "user.h"
#include "uproc.h"

static void normalrun(void);
static void dotests(void);
static void printtable(uint, const struct uproc*);
static void printtime(uint time);

int
main(int argc, char* argv[])
{
  if(1==argc)
    normalrun();
  if(1<argc)
    dotests();
  exit();
}

static void
normalrun(void)
{
  int rc;
  uint max = 64;
  struct uproc* table;

  table = malloc(sizeof(struct uproc) * max);
  if(0 == table){
    printf(2, "Error: malloc call failed. %s at line %d\n", __FILE__, __LINE__);
    exit ();
  }
  rc = getprocs(max, table);
  if(0 > rc){
    printf(2, "Error: getprocs call failed. %s at line %d\n",
	   __FILE__, __LINE__);
    free((void*)table);
    exit ();
  }
  printtable(rc, table);
  free((void*)table);
  exit();
}

static void
dotests(void)
{
  int max[] = {1, 16, 64, 72};
  int MAXCOUNT = 4;
  int i, rc;
  uint m;
  struct uproc* table;
  for (i=0; i<MAXCOUNT; i++) {
    printf(1, "\n**** MAX: %d\n", max[i]);
    table = malloc(sizeof(struct uproc) * max[i]);
    m = max[i];
    if(0 == table){
      printf(2, "Error: malloc call failed. %s at line %d\n",
	     __FILE__, __LINE__);
      exit();
    }
    rc = getprocs(m, table);
    if(0 > rc){
      printf(2, "Error: getprocs call failed. %s at line %d\n",
    	     __FILE__, __LINE__);
      free((void*)table);
      exit();
    }
    printtable(rc, table);
    free((void*)table);
  }
}
/* Prints the information in the uprocs table */
/* with one process per line. */

static void
printtable(uint num, const struct uproc* table)
{
  int i;
  printf(2, "PID\tName\t\tUID\tGID\tPPID\t");
  #ifdef CS333_P3P4
  printf(2, "Prio\t");
  #endif
  printf(2,"Elapsed\tCPU\tState\tSize\n");
  for (i = 0; i < num; i++){
    printf(2, "%d\t", table[i].pid);
    if(7 > strlen((char *) table[i].name))
      printf(2, "%s\t\t", table[i].name);
    else
      printf(2, "%s\t", table[i].name);	 
    printf(2, "%d\t%d\t%d\t",
	   table[i].uid,
	   table[i].gid,
	   table[i].ppid);
    #ifdef CS333_P3P4
    printf(2, "%d\t", table[i].prio);
    #endif
    printtime(table[i].elapsed_ticks);
    printtime(table[i].CPU_total_ticks);
    printf(2, "%s\t%d\n", table[i].state, table[i].size);
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
    printf(2, "%d.0%d\t", q, r);
  else if (10 > r)
    printf(2, "%d.00%d\t", q, r);
  else
    printf(2, "%d.%d\t", q, r);
}
#endif
