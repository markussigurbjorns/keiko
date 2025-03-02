#ifndef keiko_lowpass_filter_module_h
#define keiko_lowpass_filter_module_h

#include "audio_module.h"

extern AudioModuleInterface LowPassFilterModule;

enum {
    LPF_CUTOF_PARAM,
    LPF_Q_PARAM,
};

typedef struct {
    //filter state
    float x1, x2, y1, y2;

    //coefficients
    float b0, b1, b2, a1, a2;

    //parameters
    float cutoff, q;
    int sampleRate;

} LowPassFilter ;

#endif
