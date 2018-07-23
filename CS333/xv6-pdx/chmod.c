#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char *argv[])
{
  int rc = 0;
  int mode = 0;
  mode = atoo(argv[1]);
    if (mode > 1777 || mode < 0)
    {
      printf(1, "mode invalid\n");
      exit();
    }
  rc = chmod(argv[2],mode);
  if (rc < 0)
    printf(1, "chgrp failed\n");
  exit();
}

#endif
