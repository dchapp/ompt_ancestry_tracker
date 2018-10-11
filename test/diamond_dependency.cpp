#include "omp.h"
#include <stdio.h>

int init() {
  return 17;
}

int f(int x) {
  //printf("Hanging in f\n");
  //while(1);
  return x + 1;
}

int g(int x) {
  return x + 5;
}

void finalize(int y, int z) {
  printf("y = %d, z = %d\n", y, z); 
}

int main() {
  #pragma omp parallel 
  {
    #pragma omp single
    {
      int x, y, z;
      #pragma omp task depend(out: x)
      {
        x = init();
      }
      #pragma omp task depend(in: x) depend(out: y)
      {
        y = f(x);
      }
      #pragma omp task depend(in: x) depend(out: z)
      {
        z = g(x);
      }
      #pragma omp task depend(in: y, z)
      {
        finalize(y, z);
      }
    #pragma omp taskwait
    }
  }
}
