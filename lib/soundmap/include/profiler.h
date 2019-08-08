#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include <dirent.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <assert.h> 
#include <string.h> 
#include <unistd.h> 
#include <stdint.h> 
#include <inttypes.h> 
#include <math.h> 
#include <complex.h>

#include "transform.h"

// File parsing macros 
#define MAX_FILEPATH_SIZE 1024
#define MAX_WAV_FILE_SIZE 47088026 //524288 // Allow 50MB max file size 
#define FMT_STR_SIZE 4 // number of bytes in fmt string 

// Booleans because i'm a clean ass programmer
#define TRUE 1 
#define FALSE 0 

// spectrogram generating macros 
#define SGRAM_CHUNK_SIZE 1024 // chunk size in samples THIS SHOULD BE A POWER OF 2
#define SGRAM_CHUNK_BYTES(input) SGRAM_CHUNK_SIZE * input // chunk size in bytes when input sample size
#define SGRAM_CHUNK_OVERLAP(input) 0.5 * input // overlap in bytes when input chunksize * samplesize
#define WINDOW_FUNCTION(input,size) abs(input - (2*size)) 

#define ALIGN_2(x) pow(2,ceil(log2(x))) // align to the power of 2 >= x
#define FRAME_SIZE 0.025 // Frame size in seconds
#define FRAME_STEP 0.010 // Frame step size in seconds 

// Conversion macros between mels and frequencies
#define FREQ_TO_MEL(f) 1125 * log(1 + (double) f/700)
#define MEL_TO_FREQ(m) 700*(cexp((m/1125)) - 1)

#define STD_MIN_F 300 // standard frequency lower bound  
#define STD_MAX_F 8000 // standard frequency upper bound 
#define STD_FB_SIZE 26 // standard filterbank size

// Wave file header struct I found online https://en.wikipedia.org/wiki/WAV
typedef struct WAV_HEADER { 
    unsigned char riff[FMT_STR_SIZE];                      	// RIFF str (4 bytes)
    uint32_t full_size;                     				// The size of the wav file in bytes 
    unsigned char wave[FMT_STR_SIZE];                      	// WAVE string
	unsigned char fmt_chunk_marker[FMT_STR_SIZE];          	// fmt string with trailing null char
	uint32_t length_of_fmt;                 				// length of the format data
	uint16_t format_type;                   				// format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	uint16_t channels;                      				// no.of channels
	uint32_t sample_rate;                   				// sampling rate (blocks per second)
	uint32_t byterate;                      				// SampleRate * NumChannels * BitsPerSample/8
	uint16_t sample_size;                   				// NumChannels * BitsPerSample/8
	uint16_t bits_per_sample;               				// bits per sample, 8- 8bits, 16- 16 bits etc
	unsigned char data_chunk_header[4];        				// DATA string or FLLR string
	uint32_t chunk_size;                     				// NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
} wav_header_t; 

// Wave file structure 
typedef struct WAV_FILE { 
	unsigned char name[MAX_FILEPATH_SIZE]; 					// the name of the file 
	wav_header_t header; 									// the header of the wav file 
	unsigned char data[MAX_WAV_FILE_SIZE]; 					// sound data in the wav file 
} wav_file_t;

// Wav file node element 
typedef struct WAV_FILE_LIST_ELEMENT { 
	wav_file_t *node; 
	struct WAV_FILE_LIST_ELEMENT *next; 
	struct WAV_FILE_LIST_ELEMENT *prev; 
} wfl_node_t; 

// Circular double lnked list of wav files 
typedef struct WAV_FILE_LIST { 
	size_t size; 
	wfl_node_t *head;  
	wfl_node_t *tail; 
} wfl_t ; 

// Initialize a pointer to a wav file list struct 
#define wfl_init(wfl) do { 												\
	wfl->size = 0; 														\
	wfl->head = wfl->tail = NULL; 										\
} while (0) 

// Initialize a new wav file list node 
#define wfl_node_init(wfl_node) do { 									\
	wfl_node->node = (wav_file_t *) malloc(sizeof(wav_file_t)); 		\
	wfl_node->next = wfl_node->prev = NULL; 							\
} while (0)

// wfl should be pointer to wfl and node should be the actual node
#define wfl_insert_head(wfl,node) do { 									\
	node->next = wfl->head; 											\
	node->prev = NULL; 													\
	wfl->head = node;													\
	wfl->size++; 														\
	if (wfl->size == 1) {wfl->tail=wfl->head;}							\
} while (0)											

#define wfl_insert_tail(wfl,node) do { 									\
	node->next = NULL; 													\
	node->prev = wfl->tail; 											\
	wfl->tail = node;													\
	wfl->size++;														\
	if (wfl->size == 1) {wfl->head=wfl->tail;}							\
} while (0)

// clean wav file list and nodes 
#define wfl_clean(wfl, node) do {                   					\
    wfl_node_t *curr = wfl->head                    					\
    wfl_node_t *old;                                					\
    while(curr) {                                   					\
        old = curr;                                 					\
        curr = curr->next;                          					\
        free(old);                                  					\
    }                                               					\
    free(wfl);                                      					\
} while (0)

// This is the 
int load_wav_dir(const char* dirname, wfl_t *results, size_t* numwav);
int gen_sgram_16(wav_file_t *input, complex_t **output); 
int feature_print(wav_file_t *input, char *out); 
