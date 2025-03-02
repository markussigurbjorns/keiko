#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "lowpass_filter_module.h"
#include "audio_module.h"

static void compute_coefficients(LowPassFilter * filter);

static void* create(void) {
    LowPassFilter* filter = (LowPassFilter*)calloc(1, sizeof(LowPassFilter));
    if (!filter) {
        fprintf(stderr, "Failed to allocate filter\n");
        return NULL;
    }
    filter->cutoff = 1000.0f;
    filter->q = 0.7071f;
    return filter;
}

static void destroy(void* instance) {
    free(instance);
}

static void init(void* instance, int sampleRate, int bufferSize) {
    LowPassFilter* filter = (LowPassFilter*) instance;
    filter->sampleRate = sampleRate;
    compute_coefficients(filter);
}

static void process(void* instance, const float* input, float* output, int numSamples) {
    LowPassFilter* filter = (LowPassFilter*) instance;

    float x1 = filter->x1;
    float x2 = filter->x2;
    float y1 = filter->y1;
    float y2 = filter->y2;

    const float b0 = filter->b0;
    const float b1 = filter->b1;
    const float b2 = filter->b2;
    const float a1 = filter->a1;
    const float a2 = filter->a2;

    for (int i=0; i<numSamples; i++) {
        const float x = input[i];
        const float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;

        output[i] = y;
    }

    filter->x1 = x1;
    filter->x2 = x2;
    filter->y1 = y1;
    filter->y2 = y2;
}

static void setParameter(void* instance, int parameterId, float value){
    LowPassFilter* filter = (LowPassFilter*)instance;
    switch (parameterId){
        case LPF_CUTOF_PARAM:
            filter->cutoff = fmax(20.0f, fmin(value, 20000.f));
            compute_coefficients(filter);
            break;
        case LPF_Q_PARAM:
            filter->q = fmax(0.1f, fmin(value, 10.0f));
            compute_coefficients(filter);
            break;
    }
}

static float getParameter(void* instance, int parameterId) {
    LowPassFilter* filter = (LowPassFilter*)instance;
    switch (parameterId) {
        case LPF_CUTOF_PARAM: return filter->cutoff;
        case LPF_Q_PARAM: return filter->q;
        default: return 0.0f;
    }
}

static void reset(void* instance) {
    LowPassFilter* filter = (LowPassFilter*)instance;
    filter->x1 = filter->x2 = 0.0f;
    filter->y1 = filter->y2 = 0.0f;
}

AudioModuleInterface LowPassFilterModule = {
    .create = create,
    .destroy = destroy,
    .init = init,
    .process = process,
    .setParameter = setParameter,
    .getParameter = getParameter,
    .reset = reset
};

static void compute_coefficients(LowPassFilter * filter) {
    const float omega = 2.0 * (float)M_PI * filter->cutoff / filter->sampleRate;
    const float sn = sinf(omega);
    const float cs = cosf(omega);
    const float alpha = sn / (2.0f * filter->q);

    if (filter->cutoff <= 0.0f || filter->cutoff >= (float)filter->sampleRate/2) {
        filter->b0 = 1.0f;
        filter->b1 = filter->b2 = filter->a1 = filter->a2 = 0.0f;
        return;
    }

    // Compute coefficients
    const float a0 = 1.0f + alpha;
    const float b0 = (1.0f - cs) / 2.0f;
    const float b1 = 1.0f - cs;
    const float b2 = b0;
    const float a1 = -2.0f * cs;
    const float a2 = 1.0f - alpha;

    // Normalize coefficients
    filter->b0 = b0 / a0;
    filter->b1 = b1 / a0;
    filter->b2 = b2 / a0;
    filter->a1 = a1 / a0;
    filter->a2 = a2 / a0;
}
