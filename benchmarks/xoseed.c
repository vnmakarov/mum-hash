#include <stdlib.h>
#include <stdio.h>
#include "splitmix64.c"

void main (int argc, char *argv[]) {
  int n = atoi(argv[1]);
  for (int i = 0; i < n; i++)
    printf (" s[%d] = 0x%lx;", i, next());
  printf ("\n");
}
