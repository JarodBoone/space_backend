# Sound Map 
Experimentation for the space application sound processing server.

Problem: User sends server a soundfile and we need to map this soundfile to another user 

## Implementation 
Storing blobs of sound data in the database doesn't really work and storing the individual media URLS seems weird for a problem of this nature. What we need is a way to map a user to a set of sound bytes that in turn map to other users. This can be achieved with some straightforward directory calling

Proposed Architecture: Upon user request to map soundfile we will query the user database for a filesystem path associated with that user this file system path will lead to a directory of .wav files whose title are the usernames of other users consider .wav. Django will handle the request and use the python subprocess module to call a c++ binary file that will scrape this directory with the soundfile as input. The .wav file in the directory that most closely matches the input soundfile will be selected and the user will be resolved. This username will be
used to access the database and return user information (push token etc)

The django app that will be doing the heavylifting is going to be the soundmap application. This I imagine will be some API that is accessed by a frontend application. For now I am black boxing the actual API calls and how I get the sound files. 

The majoirity of the logic will be in the c binary I build in the **/lib** directory. The idea for this is to build out a django application that I can ship with this c application and bind to the python build that djanog will run on and expose a sounmap function that calls the c application. 

## Wav file interpretation and theory 
Whether or not I like it the majority of this work is going to need to go into the quick comparison of WAV files. We have opted to not run an NLP algorithm and instead match on the sound of the input from a signal perspective, this increases robustness and the range of application down the line but for now is just going to be a bitch to build out. 

WAV files have a fairly well defined file format and I have captured this adequetly in a C header. I think long term since we are going to record the input we should fix the sample rate, bits per sample, channel etc. This will save some time in parsing the Wav file and in addition to that make comparison a lot easier. For now I am going to build a comparison algorithm that takes into account different sampling rates. 

Rather than doing my own bullshit I've decided to borrow some pointers from Shazam's research in which they conduct a dimensionality reduction from a spectrogram (a full realization of the spectra in an audio sample) to a so called "constellation map". They then fingerprint and hash the constellation map and use this as a sort of first order identifier of the audio files. A visual depiction of this process can be found below 

![Dimensionality Reduction](dimensionality_reduction.png)


### The comparison algorithm 
I Have selected to try and implement dynamic time warping as a way to identify the difference between wav files. This is aimed to try and address the problem of people speaking with different cadences. Most of our differences from the problem outlined in the Shazam paper are beneficial however the case of meter hurts us a bit. Since we can not expect a uniform "speed" at which people say the noise or the name they are trying to match on we lose the dimension of meter as a matching parameter. 

The way to get a round this is with an extratemporal matching algorithm such as dynamic timewarping. I'm not quite sure how the interaction between the dimensionality reduction and the time warping will work... I think that the spectrogram still has some notion of sampling rate however we lose phase. 

To my understanding dynamic time warping is a phase superposition that dynamically computes the difference between waveforms for different offsets.

```
int DTWDistance(s: array [1..n], t: array [1..m]) {
   DTW := array [0..n, 0..m]
 
   for i := 1 to n
     for j := 1 to m
       DTW[i, j] := infinity
   DTW[0, 0] := 0
 
   for i := 1 to n
       for j := 1 to m
           cost := d(s[i], t[j])
           DTW[i, j] := cost + minimum(DTW[i-1, j  ],    // insertion
                                       DTW[i  , j-1],    // deletion
                                       DTW[i-1, j-1])    // match
 
   return DTW[n, m]
}
```


## Requirements 
- g++ 7.0.4
- python 3 
- pip3 
- pipenv  

## Installation instructions 
We need a version of python 3.6 on the machine that has not been configured with the `--with-pymalloc` option (this obfuscates the system `malloc` command). You can usually tell if a python installation has been configured with this flag because it will have an 'm' following the name i.e. we want **/usr/lib/python3.6** not **/usr/lib/python3.6m** if we were on a UNIX system. 

The first step in the installation of the server is to compile and link the soundmapping python module with our python install. 

## Useful links 
[extending the python interface](https://docs.python.org/3/extending/extending.html)
[argument parsing](https://docs.python.org/3/c-api/arg.html#c.PyArg_ParseTuple)
[Cross Correlation](https://en.wikipedia.org/wiki/Cross-correlation)
[Dynamic Time Warping](https://en.wikipedia.org/wiki/Cross-correlation)
[wav file description](https://en.wikipedia.org/wiki/WAV)
[audio correlation](http://www.ee.columbia.edu/~dpwe/papers/Wang03-shazam.pdf)

