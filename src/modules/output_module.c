#include <stdlib.h>
#include <string.h>
#include "output_module.h"

static void* create(void){
    OutputNode* node = (OutputNode*)calloc(1, sizeof(OutputNode));
    node->channelCount = 1;
    return node;
}

static void destroy(void* instance) {
    OutputNode* node = (OutputNode*)instance;
    free(node);
}

static void init(void* instance, int sampleRate, int bufferSize) {
    OutputNode* node = (OutputNode*)instance;
    node->outputs = calloc(node->channelCount, sizeof(float*));
    for(int c = 0; c < node->channelCount; c++) {
        node->outputs[c] = malloc(bufferSize * sizeof(float));
    }
}

static void process(void* instance, const float* input, float* output, int numSamples) {
    OutputNode* node = (OutputNode*)instance;
    memcpy(node->outputs[0], input, numSamples * sizeof(float));
    
    if(node->channelCount > 1) {
        memcpy(node->outputs[1], input, numSamples * sizeof(float));
    }
}

AudioModuleInterface OutputNodeModule = {
    .create = create,
    .destroy = destroy,
    .init = init,
    .process = process,
    .setParameter = NULL,
    .getParameter = NULL,
    .reset = NULL,
};
