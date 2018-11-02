#ifdef CS333_P2
#include "types.h"
#include "user.h"

static int testuidgid(void);
static void testuid(void);
static void testgid(void);
static void testppid(void);

int
main(void)
{
  testuidgid();
  exit();
}

static int
testuidgid(void)
{
  testuid();
  testgid();
  testppid();
  return 0;
}

static void
testuid(void)
{
  uint uid;
  int  rc;

  /* UID testing including rc checks and boundary values */
  uid = getuid();
  printf (2, "Current UID is: %d\n", uid);
  printf(2, "Setting UID to to a negative number\n");
  rc = setuid(-1);
  if(0 > rc)
    printf(2, "SUCCESS: setuid call failed for incorrect UID\n");
  if(0 <= rc){
    printf(2, "FAILURE: Incorrect RC, UID should not set to negative\n");
    exit();
  }
  uid = getuid();
  printf (2, "Current UID should be unchanged. Current UID is: %d\n", uid);
  printf(2, "Setting UID to a number > 32767\n");
  rc = setuid(77777);
  if (0 > rc)
     printf(2, "SUCCESS: setuid call failed for incorrect UID\n");
  if (0 <= rc){
    printf(2, "FAILURE: Incorrect RC, UID should not set to such a large number\n");
    exit();
  }
  uid = getuid();
  printf(2, "Current UID should be unchanged. Current UID is: %d\n", uid);
  printf(2, "Setting UID to a valid number (42)\n");
  rc = setuid(42);
  if(0 <= rc)
    printf(2, "SUCCESS: setuid call returned for valid UID\n");
  if(0 > rc){
    printf(2, "FAILURE: setuid unable to set UID to valid value\n");
    exit();
  }
  uid = getuid();
  printf (2, "Current UID should be changed to 42. Current UID is: %d\n", uid);
  printf(2, "Setting UID to a valid number (32767)\n");
  rc = setuid(32767);
  if(0 <= rc)
    printf(2, "SUCCESS: setgid call returned for valid GID\n");
  if(0 > rc){
    printf(2, "FAILURE: setgid unable to set GID to valid value\n");
    exit();
  }
  uid = getuid();
  printf (2, "Current GID should be changed to 32767. Current GID is: %d\n", uid);
  printf(2, "Setting UID to a valid number (0)\n");
  rc = setuid(0);
  if(0 <= rc)
    printf(2, "SUCCESS: setgid call returned for valid GID\n");
  if(0 > rc){
    printf(2, "FAILURE: setgid unable to set GID to valid value\n");
    exit();
  }
  uid = getuid();
  printf (2, "Current GID should be changed to 0. Current GID is: %d\n", uid);  
}

static void
testgid(void)
{
  int rc;
  uint gid;
  /* GID testing with boundary values and rc checking */
  gid = getgid();
  printf (2, "Current GID is: %d\n", gid);
  printf(2, "Setting GID to to a negative number\n");
  rc = setgid(-1);
  if(0 > rc)
    printf(2, "SUCCESS: setgid call failed for incorrect GID\n");
  if(0 <= rc){
    printf(2, "FAILURE: Incorrect RC, GID should not set to negative\n");
    exit();
  }
  gid  = getgid();
  printf(2, "Current GID should be unchanged. Current GID is: %d\n", gid);
  printf(2, "Setting GID to a number > 32767\n");
  rc = setgid(77777);
  if(0 > rc)
    printf(2, "SUCCESS: setgid call failed for incorrect GID\n");
  if(0 <= rc){
    printf(2, "FAILURE: Incorrect RC, GID should not set to such a large number\n");
    exit();
  }
  gid = getgid();
  printf(2, "Current GID should be unchanged. Current GID is: %d\n", gid);
  printf(2, "Setting GID to a valid number (42)\n");
  rc = setgid(42);
  if(0 <= rc)
    printf(2, "SUCCESS: setgid call returned for valid GID\n");
  if(0 > rc){
    printf(2, "FAILURE: setgid unable to set GID to valid value\n");
    exit();
  }
  printf(2, "Setting GID to a valid number (32767)\n");
  rc = setgid(32767);
  if(0 <= rc)
    printf(2, "SUCCESS: setgid call returned for valid GID\n");
  if(0 > rc){
    printf(2, "FAILURE: setgid unable to set GID to valid value\n");
    exit();
  }
  gid = getgid();
  printf (2, "Current GID should be changed to 32767. Current GID is: %d\n", gid);
  printf(2, "Setting GID to a valid number (0)\n");
  rc = setgid(0);
  if(0 <= rc)
    printf(2, "SUCCESS: setgid call returned for valid GID\n");
  if(0 > rc){
    printf(2, "FAILURE: setgid unable to set GID to valid value\n");
    exit();
  }
  gid = getgid();
  printf (2, "Current GID should be changed to 0. Current GID is: %d\n", gid);
}

static void
testppid(void)
{
  uint ppid;
    /* PPID testing to make sure a PPID is returned */
  ppid = getppid();
  printf(2, "The parent process should be that of the shell. My parent process is: %d\n", ppid);
  
  printf(2, "All tests Succeeded!\n");

}
#endif
