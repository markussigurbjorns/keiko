#ifndef keiko_audio_module_h
#define keiko_audio_module_h

typedef struct {
    void* (*create)(void);
    void  (*destroy)(void* instance);
    void  (*init)(void* instance, int sampleRate);
    void  (*process)(void *instance, const float *input, float *output, int numSamples);
    void  (*setParameter)(void *instance, int parameterId, float value);
    float  (*getParameter)(void *instance, int parameterId);
    void  (*reset)(void *instance);
} AudioModuleInterface;

#endif
