// C functions for loading, indexing and profiling different wav files from a directory 
#include "profiler.h"

// Helper function that crawls through a directory of wav files (dirname)
// and parses the result into a wave file list which is stored in (results)
// assumes that results is a properlay allocated wav file list structures 
int load_wav_dir(const char *dirname, wfl_t *results, size_t *numwav) { 

    // sanity check on file size 
    assert(strlen(dirname) <= MAX_FILEPATH_SIZE); 
    
    struct dirent *entry; 
    DIR *directory; 

    // memory to store the current working directory 
    char *cwd = (char *) malloc(sizeof(char) * MAX_FILEPATH_SIZE); 
    if (!cwd) { 
        perror("malloc:"); 
        return -1; 
    } else if (getcwd(cwd,MAX_FILEPATH_SIZE) == NULL) { 
        perror("cwd:"); 
        free(cwd); 
        return -1;
    }

    // memory to store filename 
    char *filename = (char *) malloc(sizeof(char) * MAX_FILEPATH_SIZE); 
    if (!filename) { 
        perror("malloc:"); 
        return -1; 
    }

    // Open directory stream 
    directory = opendir(dirname);
    if (!directory) { 
        perror("opendir: "); 
        free(cwd); 
        free(filename); 
        return -1; 
    }

    // Iterate through the wav files and convert to wav_file_t and add to list
    FILE *fstream;
    wav_header_t *header = (wav_header_t *) malloc(sizeof(wav_header_t));       

    while ((entry = readdir(directory)) != NULL) { 
        // copy the dirname into the filesize
        strncpy(filename,dirname,MAX_FILEPATH_SIZE); 
        filename = strcat(filename,entry->d_name);
        
        // Open file stram for binary reading 
        fstream = fopen(filename,"rb"); 

        if (!fstream) { 
            perror("fopen"); 
            continue;
        }

        size_t read = fread((void *) header,sizeof(wav_header_t),1,fstream); 

        // If the file is mnot a Wave file obviously continue 
        if (strncmp(header->riff,"RIFF",FMT_STR_SIZE)) { 
            fclose(fstream); 
            continue; 
        }

        // need to convert to little endianess 
        unsigned char *t = (unsigned char *) &header->full_size;
        header->full_size = (t[0] |
            t[1] << 0x8 | 
            t[2] << 0x10 |
            t[3] << 0x18);

        t = (unsigned char *) &header->channels; 
        header->channels = (t[0] | 
            t[1] << 0x8 | 
            t[2] << 0x10 | 
            t[3] << 0x18);  

        t = (unsigned char *) &header->bits_per_sample; 
        header->bits_per_sample = (t[0] | 
            t[1] << 0x8 | 
            t[2] << 0x10 | 
            t[3] << 0x18);  

        printf("Wav file name %s\n",filename); 
        printf("Wav file size: %d (bytes)\n",header->full_size); 
        printf("Wav file channels: %d\n",header->channels); 
        printf("Wav file sample rate: %d Hz\n",header->sample_rate); 
        printf("Bits per sample: %d \n",header->bits_per_sample);
        printf("Sample size: %d \n",header->sample_size);

        // create a new node to add to the list 
        wfl_node_t *wfl_node = (wfl_node_t *) malloc(sizeof(wfl_node_t)); 
        wfl_node_init(wfl_node); 

        wav_file_t *wav_file = wfl_node->node; 

        // Cut off cue chunks 
        while (!strncmp(header->data_chunk_header,"cue ",FMT_STR_SIZE)) { 
            // If there is a cue before the start of the data make sure we need to 
            // read that chunksize into grabage and get the next chunk  
            read = fread(wav_file->data, 1, header->chunk_size,fstream); // this will be overwritten
            assert(read == header->chunk_size); 
        
            read = fread(header->data_chunk_header, 1,sizeof(char) * 4,fstream); 
            assert(read == (sizeof(char) * 4)); 

            read = fread(&(header->chunk_size),1,sizeof(uint32_t),fstream);
            assert(read == sizeof(uint32_t)); 
        }

        // copy name and header to wav file struct 
        memcpy((void *) &wav_file->header, (const void *) header, sizeof(wav_header_t));
        strncpy(wav_file->name,filename,MAX_FILEPATH_SIZE);

        // read the rest of the data file 
        read = fread(wav_file->data, 1, header->chunk_size,fstream); 

        wfl_insert_head(results,wfl_node); 
        
        // clear the header for reuse 
        memset((void *) header,0,sizeof(wav_header_t)); 

    }

    free(header); 
    free(cwd); 
    free(filename); 
    fclose(fstream); 

    return 0; 
}
 
// 16 bit sample size spectrogram generation 
int gen_sgram_16(wav_file_t *input,complex_t **output) { 

    // Baseline sanity check 
    assert(input != NULL); 
    
    // the size of the sample array is the total chunk size divided by the data sample length
    size_t num_samples = input->header.chunk_size/input->header.sample_size; 

    // the sample array which is the full chunksize in bytes 
    char* sample_arr = (char *) malloc(input->header.chunk_size);
    if (!sample_arr) { 
        perror("malloc"); 
        return -1; 
    }
    
    // generating sgram
    printf("generating sgram for %s\n",input->name); 
    printf("data size of %u\n",(unsigned) num_samples); 
    FILE *gnuplot = fopen("gnuplot.txt", "w");
    fprintf(gnuplot, "plot '-' with lines\n");
    for (int i = 0; i < num_samples; i++) { 
        // increment the data pointer by the chunksize and read 
        size_t chunk_index = i * input->header.sample_size; 
        memcpy((&sample_arr[chunk_index]), ((const void *) &input->data[chunk_index]),input->header.sample_size);  
        fprintf(gnuplot, "%d %" PRId16 "\n", i,sample_arr[chunk_index]); 
    }

    printf("wav file sampled\n"); 

    fprintf(gnuplot, "e\n");
    fflush(gnuplot);
    fclose(gnuplot); 
    
    // Get the chunksize and the overlap in bytes 
    uint32_t scb = (uint32_t) SGRAM_CHUNK_BYTES(input->header.sample_size); // sgram chunksize in bytes
    uint32_t sco = (uint32_t) SGRAM_CHUNK_OVERLAP(SGRAM_CHUNK_SIZE * input->header.sample_size); // overlap in bytes 
    assert(sco < scb);

    complex_t chunk[SGRAM_CHUNK_SIZE]; // chunk space padded to power of 2

    // The step size in bytes that we will iterate through the sample with
    uint32_t step_size = scb - sco;

    uint32_t frequency_map[SGRAM_CHUNK_SIZE / 2]; // a map of frequencies corresponding to indices in the processed chunk array [1:N/2]
    uint32_t time_delta = SGRAM_CHUNK_SIZE/input->header.sample_rate; // how much time each chunk processes (period)

    for (int i = 0; i < SGRAM_CHUNK_SIZE/2; i++) { 
        frequency_map[i] = (i * input->header.sample_rate)/SGRAM_CHUNK_SIZE;
    }

    // iterations of chunking we need to do (don't care about last couple samples)
    int chunk_windows = input->header.chunk_size/step_size; 
    complex_t sgram[chunk_windows][SGRAM_CHUNK_SIZE/2]; // the output spectrogram 
    
    for (int i = 0; i < chunk_windows; i++) { 
        // zero out the chunk buffer effectively padding with zeros 
        memset(((void *) chunk),0, SGRAM_CHUNK_SIZE * sizeof(complex_t)); 

        // iterate by samples over the chunk 
        for (int ii = 0; ii < SGRAM_CHUNK_SIZE; ii++) { 
            int sample_index = (ii * input->header.sample_size) + (i * step_size); 
            chunk[ii] = (complex_t) sample_arr[sample_index];
            chunk[ii] = creal(chunk[ii]);
        }

        fft(chunk,SGRAM_CHUNK_SIZE);

        for (int ii = 0; ii < SGRAM_CHUNK_SIZE/2; ii++) {
            sgram[i][ii] = creal(chunk[ii]);
        }
    }

    // output = sgram; 
    free (sample_arr); 
    return 0; 
}

/**
 * Generate mel filter banks given the input parameters. Returns a matrix of filters of the 
 * form int[num_filters][filter_size]
 * 
 * @params (num_filters) the number of filters you want to generate 
 * @params (filter_size) the number of periodogram coefficients that will be filtered by these 
 * @params (min_f) the minimum frequency 
 * @params (max_f) the maximum frequency 
 * @params (sample_rate) the sample rate of the signal we are building the filter banks for 
 * 
 */
float **generate_filter_bank(size_t num_filters,size_t filter_size,uint32_t min_f, 
    uint32_t max_f,uint32_t sample_rate, float** out) { 

    double min_mel = FREQ_TO_MEL(min_f); 
    printf("min mel %f\n",min_mel); 

    double max_mel = FREQ_TO_MEL(max_f);
    printf("max mel %f\n", max_mel);

    assert(min_mel < max_mel); 

    double step_size = (max_mel - min_mel)/(num_filters + 1); 

    double mel_vector[num_filters + 2]; 
    int bin_vector[num_filters + 2]; 

    for (int i = 0; i < num_filters + 2; i++) {
        mel_vector[i] = min_mel + (i * step_size); 
        mel_vector[i] = MEL_TO_FREQ(mel_vector[i]); 
        bin_vector[i] = floor((filter_size + 1) * mel_vector[i]/sample_rate);
        printf("%f\n",mel_vector[i]); 
        printf("To bin %d\n",bin_vector[i]); 

    }

    for (int i = 0; i < num_filters; i++) { 
        int fn = i + 1;
        for (int ii = 0; ii < filter_size; ii++) { 
            if (ii <  bin_vector[fn - 1]) out[i][ii] = 0; 
            else if (ii < bin_vector[fn]) out[i][ii] = (ii - bin_vector[fn -1])/(bin_vector[fn] - bin_vector[fn - 1]); 
            else if (ii < bin_vector[fn + 1]) out[i][ii] = (bin_vector[fn + 1] - ii)/(bin_vector[fn + 1] - bin_vector[fn]); 
            else out[i][ii] = 0; 
        }
    }
}

// generate a feature hash for the given wav file 
int feature_print(wav_file_t *input, char *out) {

    // Baseline sanity check
    assert(input != NULL);

    // The number of samples to cover the time of FRAME_SIZE given the frequency of the signal 
    assert(!(input->header.sample_rate % (uint32_t) (1/FRAME_SIZE))); // make sure sample rate splits into frames 
    uint32_t samples_per_frame = FRAME_SIZE * input->header.sample_rate; 
    printf("Samples per frame %u\n",samples_per_frame);
    samples_per_frame = ALIGN_2(samples_per_frame); // align to a power of 2 so number of samples in frame can be fft 

    uint32_t sig_samples_per_frame = samples_per_frame/2; // Only first half of the fourier coefficients are relevant

    assert(!(input->header.sample_rate % (uint32_t) (1/FRAME_STEP))); // make sure sample rate splits into steps
    uint32_t samples_per_step = FRAME_STEP * input->header.sample_rate; 
    printf("Samples per step %u\n",samples_per_step);
    assert(samples_per_frame >= samples_per_step); // make sure the frame size is greater than the step size 

    // the size of the sample array is the total chunk size divided by the data sample length
    size_t num_samples = input->header.chunk_size / input->header.sample_size; // number of samples available 
    printf("Number of samples in input data %u\n",num_samples);

    // zero padded number of samples needed to fill frames
    size_t num_samples_to_frame = num_samples + (num_samples % (samples_per_frame - samples_per_step));
    printf("Number of samples to frame %u\n",num_samples_to_frame);

    // number of frames we will get from the data    
    size_t num_steps = (num_samples_to_frame - (samples_per_frame - samples_per_step)) / samples_per_step; 
    printf("Number of steps %u\n",num_steps); 

    // the sample array which is the number of samples we need to properly frame 
    char* sample_arr = (char*)malloc(num_samples_to_frame * input->header.sample_size);
    if (!sample_arr) {
        perror("malloc");
        return -1;
    }

    memset((void *) sample_arr,0,num_samples_to_frame * input->header.sample_size); // zero padding 
    memcpy((void *) sample_arr,input->data,input->header.chunk_size); // copy the data 

    // need the number of frames to be even for the algorithm to work 
    size_t num_frames = num_steps % 2 ? num_steps + 1 : num_steps; // size of frame array in frames 
    size_t bytes_per_step = samples_per_step * input->header.sample_size; // step size in bytes 


    // Need to dynamically allocate the frame matrix or else we blow the stack
    complex_t **frames = (complex_t **) malloc(num_frames * sizeof(complex_t*));
    if (!frames) {
        perror("malloc");
        return -1;
    }

    // Allocate rows of the frame matrix 
    for(int i = 0; i < num_frames; i++) {
        frames[i] = (complex_t *) malloc(samples_per_frame * sizeof(complex_t)); 
        if (!frames[i]) {
            perror("malloc");
            return -1;
        }

        // If this is the last frame zero pad it in case we aligned up to an even number
        if (i == (num_frames - 1)) { 
            memset((void *) frames[i],0,samples_per_frame * sizeof(complex_t)); 
        }
    }

    // printf("generating feature print for %s\n", input->name);
    // printf("data size of %u\n", (unsigned)num_samples);
    // printf("sample rate of %u Hz\n",input->header.sample_rate);
    // printf("Num Steps %u\n",num_steps);
    // printf("Num Frames %u\n",num_frames);
    // printf("Samples to frame %u\n",num_samples_to_frame);

    // Generate filter bank
    float **filter_bank = malloc(sizeof(float *) * STD_FB_SIZE); 
    for (int i = 0; i < STD_FB_SIZE; i++){ 
        filter_bank[i] = malloc(sizeof(float) * sig_samples_per_frame); 
    }
    generate_filter_bank(STD_FB_SIZE,sig_samples_per_frame,STD_MIN_F,STD_MAX_F,
        input->header.sample_rate,filter_bank);

    for (int i = 0; i < num_steps; i++) {
        // increment the data pointer by frame step size 
        size_t frame_index = i * bytes_per_step;

        // Copy a frame of bytes from the sample array into the frame at this step as complex values 
        // TODO : iterate by more than one 
        for (int ii = 0; ii < samples_per_frame; ii++){ 
            // printf("sample: %d" PRId16 "\n",sample_arr[frame_index + (ii * input->header.sample_size)]); 
            // frames[i][ii] = (complex_t) (sample_arr[frame_index + (ii * input->header.sample_size)]); 
            frames[i][ii] = frames[i][ii] + 1;
        }

        // In place fast fourier transform 
        fft(frames[i],samples_per_frame); 

        // Turn the first (samples_per_frame/2) fourier coefficients into periodogram coefficients 
        pg_estimate(frames[i],sig_samples_per_frame); 
    }

    free(sample_arr); 
    for(int i = 0; i < num_frames; i++) {
        free(frames[i]);
    }
    free(frames);

    for (int i = 0; i < STD_FB_SIZE; i++) { 
        free(filter_bank[i]); 
    }
    free(filter_bank); 
    


}

// Generate a list of sound profiles and return a pointer to it 
// sprofile 

// int main(int argc, char **argv) {

//     FILE* fp;
//     FILE* fp2;

//     /*Array with file data*/
//     unsigned char* buffer;

//     /*Array used for converting two bytes in an unsigned int*/
//     unsigned char ub[2];

//     /*The unsigned int obtained*/
//     uint16_t* conv;

//     /*The new value calculated*/
//     uint16_t nval;

//     /*Array used for the reverse conversion, form UINT to bytes*/
//     unsigned char* nvalarr;

//     for (i = 44; i < BUFFER_SIZE; i++) {

//         if (i % 2 == 0) {

//             /*I read 2 bytes form the array and "convert" it in an unsigned int*/
//             ub[0] = buffer[i];
//             ub[1] = buffer[i + 1];

//             conv = (uint16_t*)&ub[0];

//             /*Calculate the new value (-30%) to write in the new file*/

//             nval = *conv - ((float)*conv * 30 / 100);
//             if (nval < 0)
//                 nval = 0;

//             nvalarr = malloc(2);
//             memset(nvalarr, '\0', 2);
//             nvalarr = (unsigned char*)&nval;

//             /*Write the two bytes of the new file*/
//             fwrite(&nvalarr[0], 1, 1, fp2);
//             fwrite(&nvalarr[1], 1, 1, fp2);
//         }
//     }

//     int sampleCount = ((fileSize - dataOffset) / sizeof(int16_t));
//     int16_t* samp = (int16_t*)&fileBuffer[dataOffset];
//     float percent = 0.6f;

//     for (int i = 0; i < sampleCount; i++) {
//         samp[i] -= (int16_t)(samp[i] * percent); // Should work +/- values
//     }
// }