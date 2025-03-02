#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "sine_osc_module.h"


static void* create(void) {
    SineOscillator* osc = (SineOscillator*)calloc(1, sizeof(SineOscillator));
    if (!osc) {
        fprintf(stderr, "Failed to allocate osc");
        return NULL;
    }
    osc->frequency = 440.0f;
    osc->gain = 0.2;
    osc->phase = 0.0f;
    return osc;
}

static void destroy(void* instance) {
    SineOscillator* osc = (SineOscillator*)instance;
    free(osc);
}

static void init(void* instance, int sampleRate, int bufferSize) {
    SineOscillator* osc = (SineOscillator*)instance;
    osc->sampleRate = sampleRate;
}

static void process(void* instance, const float *input, float *output, int numSamples) {
    SineOscillator* osc = (SineOscillator*)instance;
    (void)input;
    const float phaseIncrement = (2.0f * (float)M_PI * osc->frequency) / osc->sampleRate;
    
    for(int i = 0; i < numSamples; i++) {
        output[i] = sinf(osc->phase) * osc->gain;
        
        osc->phase += phaseIncrement;
        if(osc->phase >= 2.0f * (float)M_PI) {
            osc->phase -= 2.0f * (float)M_PI;
        }
    }
}

static void setParameter(void* instance, int parameterId, float value) {
    SineOscillator* osc = (SineOscillator*)instance;
    switch(parameterId) {
        case OSC_FREQUENCY_PARAM:
            osc->frequency = value;
            break;
        case OSC_GAIN_PARAM:
            osc->gain = value;
            break;
    }
} 

static float getParameter(void* instance, int parameterId) {
    SineOscillator* osc = (SineOscillator*)instance;
    switch(parameterId) {
        case OSC_FREQUENCY_PARAM: return osc->frequency;
        case OSC_GAIN_PARAM: return osc->gain;
        default: return 0.0f;
    }
}

static void reset(void* instance) {
    SineOscillator* osc = (SineOscillator*)instance;
    osc->phase = 0.0f;
}

AudioModuleInterface SineOscillatorModule = {
    .create = create,
    .destroy = destroy,
    .init = init,
    .process = process,
    .setParameter = setParameter,
    .getParameter = getParameter,
    .reset = reset
};
