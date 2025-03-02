#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <portaudio.h>
#include <pthread.h>
#include <stdlib.h>

#include "audio_graph.h"
#include "sine_osc_module.h"
#include "lowpass_filter_module.h"
#include "output_module.h"

#define SAMPLE_RATE 44100
#define NUM_SECONDS 20
#define SINE_FREQ   440.0f
#define AMPLITUDE   0.2f
#define MOD_FREQ    10.0f
#define MOD_DEPTH   0.5f
#define FRAMES_PER_BUFFER 128
#define RING_BUFFER_SIZE (FRAMES_PER_BUFFER * 8+1)

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

static void* audioProcessingThread(void *args) {

    AudioGraph* graph = (AudioGraph*)args;

    OutputNode* out = (OutputNode*)graph->nodes[graph->numNodes-1]->instance;

    while (running) {

        process_graph(graph, FRAMES_PER_BUFFER);

        while (!writeAtomicRingBuffer(&rb, out->outputs[0], FRAMES_PER_BUFFER) && running) {
            //buffer is full
            Pa_Sleep(1);
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

    static float lastBuffer[FRAMES_PER_BUFFER];
    static bool hasLastBuffer = false; 

    // I am not sure about this handling of buffer underflow
    if (!readAtomicRingBuffer(&rb, out, framesPerBuffer)) {
        if (hasLastBuffer) {
            memcpy(out, lastBuffer, framesPerBuffer * sizeof(float));
        } else {
            memset(out, 0, framesPerBuffer * sizeof(float));
        }
    } else {
        memcpy(lastBuffer, out, framesPerBuffer * sizeof(float));
        hasLastBuffer = true;
    }
    return 0;
}


int main(){
    PaError err;
    PaStream *stream;

    initAtomicRingBuffer(&rb, RING_BUFFER_SIZE);

    AudioGraph* graph = create_audio_graph();

    AudioNode* sine_osc = create_audio_node(&SineOscillatorModule);
    add_node(graph, sine_osc);

    sine_osc->interface->setParameter(sine_osc->instance, OSC_FREQUENCY_PARAM, 220.0f);
    sine_osc->interface->setParameter(sine_osc->instance, OSC_GAIN_PARAM, 0.4);

    AudioNode* sine_osc_2 = create_audio_node(&SineOscillatorModule);
    add_node(graph, sine_osc_2);

    sine_osc_2->interface->setParameter(sine_osc_2->instance, OSC_FREQUENCY_PARAM, 261.626f);
    sine_osc_2->interface->setParameter(sine_osc_2->instance, OSC_GAIN_PARAM, 0.4);    

    AudioNode* sine_osc_3 = create_audio_node(&SineOscillatorModule);
    add_node(graph, sine_osc_3);

    sine_osc_3->interface->setParameter(sine_osc_3->instance, OSC_FREQUENCY_PARAM, 329.628f);
    sine_osc_3->interface->setParameter(sine_osc_3->instance, OSC_GAIN_PARAM, 0.4);

    AudioNode* lpf = create_audio_node(&LowPassFilterModule);
    add_node(graph, lpf);

    lpf->interface->setParameter(lpf->instance, LPF_CUTOF_PARAM, 300.0f);

    AudioNode* out = create_audio_node(&OutputNodeModule);
    add_node(graph, out);

    connect_nodes(graph, sine_osc, lpf);
    connect_nodes(graph, sine_osc_2, lpf);
    connect_nodes(graph, sine_osc_3, lpf);
    connect_nodes(graph, lpf, out);

    init_graph(graph, SAMPLE_RATE, FRAMES_PER_BUFFER);

    err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }
    //printf("PortAudio version: %s\n", Pa_GetVersionText());

    pthread_t audioThread;
    pthread_create(&audioThread, NULL, audioProcessingThread, graph);

    err = Pa_OpenDefaultStream(&stream,
                               0, 
                               1,
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

    destroy_audio_graph(graph);
    
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
