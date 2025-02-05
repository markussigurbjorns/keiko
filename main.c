#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <portaudio.h>
#include <sched.h>
#include <pthread.h>
#include <stdlib.h>

#define SAMPLE_RATE 44100
#define NUM_SECONDS 9
#define SINE_FREQ   440.0f
#define AMPLITUDE   0.2f
#define FRAMES_PER_BUFFER 8192
#define RING_BUFFER_SIZE (FRAMES_PER_BUFFER * 4 +1)


/* ATOMIC RINGBUFFER*/

typedef struct {
    float *buffer;
    int size;
    atomic_int readPos;
    atomic_int writePos;
} AtomicRingBuffer ;

static void initAtomicRingBuffer(AtomicRingBuffer *rb, int size) {
    rb->buffer = (float*)malloc(size* sizeof(float));
    rb->size = size;
    atomic_init(&rb->readPos, 0);
    atomic_init(&rb->writePos, 0);
}

static void freeAtomicRingBuffer(AtomicRingBuffer *rb) {
    free(rb->buffer);
}

static bool writeAtomicRingBuffer(AtomicRingBuffer *rb, float *data, int numSamples) {
    int readPos = atomic_load_explicit(&rb->readPos, memory_order_acquire);
    int writePos = atomic_load_explicit(&rb->writePos, memory_order_relaxed);
    
    int available = (readPos > writePos) ? (readPos - writePos) : (rb->size - writePos + readPos);

    if (available < numSamples) return false;

    int spaceToEnd = rb->size - writePos;

    if (spaceToEnd >= numSamples) {
        memcpy(&rb->buffer[writePos], data, numSamples * sizeof(float));
    } else {
        memcpy(&rb->buffer[writePos], data, spaceToEnd * sizeof(float));
        memcpy(&rb->buffer[0], &data[spaceToEnd], (numSamples - spaceToEnd) * sizeof(float));
    }

    atomic_store_explicit(&rb->writePos, (writePos + numSamples) % rb->size, memory_order_release);
    return true;

}

static bool readAtomicRingBuffer(AtomicRingBuffer *rb, float *data, int numSamples) {
    int readPos = atomic_load_explicit(&rb->readPos, memory_order_relaxed);
    int writePos = atomic_load_explicit(&rb->writePos, memory_order_acquire);

    int available = (writePos >= readPos) ? (writePos - readPos) : (rb->size -readPos + writePos);

    if (available < numSamples) return false;

    int spaceToEnd = rb->size - readPos;

    if (spaceToEnd >= numSamples) {
        memcpy(data, &rb->buffer[readPos], numSamples * sizeof(float));
    } else {
        memcpy(data, &rb->buffer[readPos], spaceToEnd * sizeof(float));
        memcpy(&data[spaceToEnd], &rb->buffer[0], (numSamples - spaceToEnd) * sizeof(float));
    }

    atomic_store_explicit(&rb->readPos, (readPos + numSamples) % rb->size, memory_order_release);
    return true;
}

AtomicRingBuffer rb;

/***********************/
/* AUDIO PROCESSING THREAD */

volatile bool running = true;

static void *audioProcessingThread(void *args) {

    float buffer[FRAMES_PER_BUFFER*4]; // stereo

    static float right_phase = 0.0f;
    static float left_phase = 0.0f;

    float leftPhaseIncrement = (2.0f * (float)M_PI * SINE_FREQ) / (float)SAMPLE_RATE; 
    float rightPhaseIncrement = leftPhaseIncrement * 0.8;
    
    while (running) {
        for (int i=0; i<FRAMES_PER_BUFFER*4;i+=2) {
            buffer[i]   = AMPLITUDE * sinf(left_phase);
            buffer[i+1] = AMPLITUDE * sinf(right_phase);

            left_phase += leftPhaseIncrement;
            if (left_phase >= (2.0f * M_PI)) left_phase -= 2.0f * M_PI;

            right_phase += rightPhaseIncrement;
            if (right_phase >= (2.0) * M_PI) right_phase -= 2.0f * M_PI;
        }

        while (!writeAtomicRingBuffer(&rb, buffer, FRAMES_PER_BUFFER*4) && running) {
            Pa_Sleep(100);
            //sched_yield();
            //buffer is full
        }
    }
    return NULL;
}
/*********************/


typedef struct {
    float left_phase;
    float right_phase;
    float rightPhaseIncrement;
    float leftPhaseIncrement;
} paUserData;


paUserData data;

static int patestCallback(const void *inputBuffer,
                          void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeinfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData
                          ) {
    paUserData *udata = (paUserData*)userData;
    float *out = (float*)outputBuffer;

    (void)inputBuffer; /* Prevent unused variable warning. */
    (void)timeinfo;
    (void)statusFlags;

    static float lastBuffer[FRAMES_PER_BUFFER * 2];
    static bool hasLastBuffer = false; 

    if (!readAtomicRingBuffer(&rb, out, framesPerBuffer * 2)) {
        if (hasLastBuffer) {
            memcpy(out, lastBuffer, framesPerBuffer * 2 * sizeof(float));
        } else {
            memset(out, 0, framesPerBuffer * 2 * sizeof(float));
        }
    } else {
        memcpy(lastBuffer, out, framesPerBuffer * 2 * sizeof(float));
        hasLastBuffer = true;
    }
    return 0;
}


int main(){
    PaError err;
    PaStream *stream;

    initAtomicRingBuffer(&rb, RING_BUFFER_SIZE);

    err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }
    //printf("PortAudio version: %s\n", Pa_GetVersionText());

    pthread_t audioThread;
    pthread_create(&audioThread, NULL, audioProcessingThread, NULL);

    err = Pa_OpenDefaultStream(&stream,
                               0, 
                               2,
                               paFloat32,
                               SAMPLE_RATE, 
                               FRAMES_PER_BUFFER, 
                               patestCallback, 
                               &data);
    if(err != paNoError) {
        fprintf(stderr, "PortAudio error (open): %s\n", Pa_GetErrorText(err));
        Pa_Terminate();
        return 1;
    }

    err = Pa_StartStream(stream);
    if(err != paNoError) {
        fprintf(stderr, "PortAudio error (start): %s\n", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    Pa_Sleep(NUM_SECONDS*1000);

    err = Pa_StopStream(stream);
    if(err != paNoError) {
        fprintf(stderr, "PortAudio error (stop): %s\n", Pa_GetErrorText(err));
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    
    err = Pa_Terminate();
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    printf("PortAudio terminated successfully.\n");

    running = false;
    pthread_cancel(audioThread); 
    pthread_join(audioThread, NULL);
    freeAtomicRingBuffer(&rb);

    return 0;
}
