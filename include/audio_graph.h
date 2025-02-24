#ifndef keiko_audio_graph_h
#define keiko_audio_graph_h

#include "audio_module.h"

typedef struct Connection Connection;
typedef struct AudioNode AudioNode;
typedef struct AudioGraph AudioGraph;

struct AudioNode {
    void* instance;
    AudioModuleInterface* interface;
    float *inputBuffer;
    float *outputBuffer;
    Connection **incoming;
    Connection **outgoing;
    int numIncoming;
    int numOutgoing;
};

struct Connection {
    AudioNode *source;
    AudioNode *destination;
};

struct AudioGraph {
    AudioNode **nodes;
    Connection **connections;
    int numNodes;
    int numConnections;
};

AudioGraph* create_audio_graph(void); 
void destroy_audio_graph(AudioGraph *graph);
AudioNode* create_audio_node(AudioModuleInterface *interface);
void add_node(AudioGraph *graph, AudioNode *node);
void connect_nodes(AudioGraph *graph, AudioNode *src, AudioNode *dest);
void init_graph(AudioGraph *graph, int sampleRate, int bufferSize);
void process_graph(AudioGraph *graph, int numSamples);

#endif 
