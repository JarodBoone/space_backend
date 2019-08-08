#include <math.h> 
#include <stdlib.h>
#include <complex.h>
#include <unistd.h> 

typedef double complex complex_t;
#define PI 3.1415926535897932384

void fft(complex_t buf[], int buffer_size);
void pg_estimate(complex_t buf[], int buffer_size); 