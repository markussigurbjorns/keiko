#ifndef keiko_sine_osc_module_h
#define keiko_sine_osc_module_h

#include "audio_module.h"

extern AudioModuleInterface SineOscillatorModule;

enum {
    OSC_FREQUENCY_PARAM,
    OSC_GAIN_PARAM,
};

typedef struct {
    float phase;
    float frequency;
    float gain;
    int sampleRate;
} SineOscillator;

#endif
