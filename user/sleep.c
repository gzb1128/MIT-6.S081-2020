#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int n;
  if(argc!=2)
  {
      fprintf(2,"error on the argument number\n");
      exit(1);
  }
  n=atoi(argv[1]);
  if(n<=0)
      exit(1);
  sleep(n);
  exit(0);
}
