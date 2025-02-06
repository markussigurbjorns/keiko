#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "audioInterface.h"


#define ALLOCATE_OSC(type, objectType) \
    (type*)allocateOSC(sizeof(type), objectType)


static OSC* allocateOSC(size_t size, OscType type) {
    OSC* osc = (OSC*)realloc(NULL, size);
    osc->type = type;
    return osc;
}

SineOsc* allocateSineOsc(float freq) {
    SineOsc* sine = ALLOCATE_OSC(SineOsc, Sine);
    sine->freq=freq;
    return sine;
}


