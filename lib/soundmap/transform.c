#include "transform.h"

// Recursive implementation of the cooley-tukey fast fourier transform 
// simply iterate through half of the buffer and use waveform symmetry 
// for the other https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukeyfft_recursive_step_algorithm
void fft_recursive_step(complex_t input[], complex_t buffer[], int buffer_size, int step) {
	if (step < buffer_size) {

		// Recur by order 2 
		fft_recursive_step(buffer, input, buffer_size, step * 2);
		fft_recursive_step(buffer + step, input + step, buffer_size, step * 2);
 
		for (int i = 0; i < buffer_size; i += 2 * step) {
			complex_t t = cexp(-I * PI * i / buffer_size) * buffer[i + step];
			input[i / 2]     = buffer[i] + t;
			input[(i + buffer_size)/2] = buffer[i] - t;
		}
	}
}
 
/**
 * In place fast fourier transform. Assumes that buffer_size is a power of 2
 * Note that only the first buffer_size/2 frequencies will be relevant the rest 
 * will be inverse. 
 *
 * @params (input) a complex_t array of (buffer_size)
 * @params (buffer_size) the size of the buffer to transform
 * 
 */ 
void fft(complex_t input[], int buffer_size) {
	complex_t buffer[buffer_size];

	// copy the input data to an intermediate buffer 
	for (int i = 0; i < buffer_size; i++) buffer[i] = input[i];
 
	fft_recursive_step(input, buffer, buffer_size, 1);
}

/**
 * In place periodogram estimate of a signal. This is simply taking the complex conjugate of the 
 * fourier transform and dividing by the size of the transform. Using welch's method 
 * https://en.wikipedia.org/wiki/Welch%27s_method
 * 
 * Assume entirely real input array 
 */
void pg_estimate(complex_t input[], int buffer_size) {
	for (int i = 0; i < buffer_size; i++) input[i] = (complex_t) (pow(creal(input[i]),2)/buffer_size); 
}