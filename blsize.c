#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>
#include <time.h>

#define N 100

int main(){
    // Time fns
    volatile uint64_t t1, t2,latency[N];
    t1 = 0; t2 = 0;
    // Char to access every byte
    char *a = (char *)malloc(N*sizeof(char));
    volatile char temp;
    
    for (int i = 0; i < N; i++){
        a[i] = 0;
        latency[i] = 0; 
    }
  // Run tests 100 times to improve accuracy
    for(int j = 0; j < 100; j++){

        for (int i = 0; i<N; i++){
            _mm_mfence();
            _mm_clflush(&a[i]);
            _mm_mfence();
        }

        for (int i = 0; i<N; i++){
            _mm_mfence();
            t1 = _rdtsc();
            _mm_mfence();
            temp = a[i];
            _mm_mfence();
            t2 = _rdtsc();
            _mm_mfence();
            latency[i] += (t2 - t1);
            _mm_mfence();
        }
      }
    // printing latency values
    for (int i = 0; i<N; i++)
        printf("%d  %lu \n", i, latency[i]/100);
    // Free
    free(a);
    return 0;
}
