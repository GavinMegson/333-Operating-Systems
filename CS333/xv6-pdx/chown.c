#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char *argv[])
{
  int rc = 0;
  rc = chown(argv[2],atoi(argv[1]));
  if (rc < 0)
    printf(1, "chown failed\n");
  exit();
}

#endif
