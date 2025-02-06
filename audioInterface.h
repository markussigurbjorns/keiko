
typedef enum {
    Sine,
    Square
} OscType;

typedef struct {
    OscType type;
} OSC;

typedef struct {
    OSC osc;
    float freq;
} SineOsc;

typedef void (*InitFn)(OSC * osc, float freq);
typedef void (*DestroyFn)(OSC * osc);
typedef void (*StartFn)(OSC * osc);
typedef void (*StopFn)(OSC * osc);
typedef void (*ProcessFn)(OSC * osc, float * buffer);

typedef struct {
    InitFn init;
    DestroyFn destroy;
    StartFn start;
    StopFn stop;
    ProcessFn process;
} AudioInterface;

SineOsc* allocateSineOsc(float freq);
