#ifndef keiko_output_module_h
#define keiko_output_module_h

#include "audio_module.h"

extern AudioModuleInterface OutputNodeModule;

enum {
    OUTPUT_COUNT
};

typedef struct {
    float** outputs;
    int channelCount;
} OutputNode;

#endif
