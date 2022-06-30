#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>
#include <time.h>

// 32KB L1d Cache
#define CSIZE  64*1024

int main(){
    // Expected Max associativity to check for : 16 , so 16 + 1 = 17
    int N = 17;
    int offset;
    
    uint64_t t1, t2,lat[N];
    t1 = 0; t2 = 0;
    // Char to access every byte
    char *arr = (char *)malloc(N*CSIZE*sizeof(char));
    volatile char tmp;
    // Initialise
    for(int i = 0; i < N; i++)
        lat[i] = 0;
    for(int j = 0; j<N;j++){  
            tmp = arr[j*CSIZE];

        for(int i = 0; i< j; i++){
            offset = (j-i)*CSIZE;
            t1 = _rdtsc();
            _mm_mfence();
            tmp = arr[offset];
            _mm_mfence();
            t2 = _rdtsc();
            _mm_mfence();
            lat[i] += (t2 - t1);
        }
        for(int i =0; i<j;i++)
        {
          printf("%d \n",lat[i]);
        }
        printf("\n");
      }

  free(arr);
  return 0;
}
