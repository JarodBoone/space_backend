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
 
// performs an in place fast fourier transform of size buffer_size 
void fft(complex_t input[], int buffer_size) {
	complex_t buffer[buffer_size];

	// copy the input data to an intermediate buffer 
	for (int i = 0; i < buffer_size; i++) buffer[i] = input[i];
 
	fft_recursive_step(input, buffer, buffer_size, 1);
}