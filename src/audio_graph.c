#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "audio_graph.h"
#include "audio_module.h"

static void free_node(AudioNode *node);
static void free_connection(Connection *conn);
static AudioNode** topological_sort(AudioGraph *graph);

AudioGraph* create_audio_graph(void) {
    AudioGraph *graph = (AudioGraph*)malloc(sizeof(AudioGraph));
    if (!graph) {
        fprintf(stderr, "Failed to allocate audio graph\n");
        return NULL;
    }
    graph->nodes = NULL;
    graph->connections = NULL;
    graph->numNodes = 0;
    graph->numConnections = 0;
    return graph;
}

void destroy_audio_graph(AudioGraph *graph) {
    if (!graph) {return;}
    
    for (int i = 0; i < graph->numNodes; i++) {
        free_node(graph->nodes[i]);
    }
    free(graph->nodes);

    for (int i = 0; i < graph->numConnections; i++) {
        free_connection(graph->connections[i]);
    }
    free(graph->connections);
    free(graph);
}

AudioNode* create_audio_node(AudioModuleInterface *interface){
    AudioNode *node = (AudioNode*)malloc(sizeof(AudioNode));
    if (!node) {
        fprintf(stderr, "Failed to allocate node\n");
        return NULL;
    }
    node->instance = interface->create();
    if (!node) {
        fprintf(stderr, "Failed to create node\n");
        free(node);
        return NULL;
    }

    node->interface = interface;
    node->inputBuffer = NULL;
    node->outputBuffer = NULL;
    node->incoming = NULL;
    node->outgoing = NULL;
    node->numIncoming = 0;
    node->numOutgoing = 0;
    return node;
}

void add_node(AudioGraph *graph, AudioNode *node) {
    if (!graph || !node) {return;}

    graph->nodes = realloc(graph->nodes, (graph->numNodes+1)*sizeof(AudioNode*));
    if (!graph->nodes) {
        fprintf(stderr, "Failed to expand node array\n");
        return;
    }
    graph->nodes[graph->numNodes++] = node;
}

void connect_nodes(AudioGraph *graph, AudioNode *src, AudioNode *dest) {
    if (!graph || !src || !dest) {return;}

    Connection* conn = (Connection*)malloc(sizeof(Connection));
    if (!conn) {
        fprintf(stderr, "Failed to create a connection\n");
        return;
    }
    conn->source = src;
    conn->destination = dest;

    graph->connections = realloc(graph->connections, (graph->numConnections+1)*sizeof(Connection));
    if (!graph->connections) {
        fprintf(stderr, "failed to expand connection array");
        free(conn);
        return;
    }
    graph->connections[graph->numConnections++] = conn;

    src->outgoing = realloc(src->outgoing, 
        (src->numOutgoing + 1) * sizeof(Connection*));
    if (!src->outgoing) {
        free(conn);
        return;
    }
    src->outgoing[src->numOutgoing++] = conn;

    dest->incoming = realloc(dest->incoming, 
        (dest->numIncoming + 1) * sizeof(Connection*));
    if (!dest->incoming) {
        free(conn);
        return;
    }
    dest->incoming[dest->numIncoming++] = conn;
}

void init_graph(AudioGraph *graph, int sampleRate, int bufferSize) {
    if (!graph) {return;}

    for (int i = 0; i < graph->numNodes; i++){
        AudioNode* node = graph->nodes[i];

        if (node->interface->init) {
            node->interface->init(node->instance, sampleRate, bufferSize);
        }

        node->inputBuffer = realloc(node->inputBuffer, bufferSize*sizeof(float));
        node->outputBuffer = realloc(node->outputBuffer, bufferSize*sizeof(float));

        if (!node->inputBuffer || !node->outputBuffer) {
            fprintf(stderr,"Failed to allocate node buffers\n"); 
            return;
        }
        memset(node->inputBuffer, 0, bufferSize*sizeof(float));
        memset(node->outputBuffer, 0, bufferSize*sizeof(float));
    }
    //graph->nodes = topological_sort(graph);
}

void process_graph(AudioGraph *graph, int numSamples) {
    if (!graph || numSamples == 0) {return;}
    
    //AudioNode** processing_order = topological_sort(graph);
    //if (!processing_order) {
    //    fprintf(stderr, "Graph contains cycles or sorting failures");
    //    return;
    //}

    for (int i = 0; i<graph->numNodes; i++) {
        AudioNode *node = graph->nodes[i];

        memset(node->inputBuffer, 0, numSamples*sizeof(float));

        for (int j = 0; j < node->numIncoming; j++) {
            Connection *conn = node->incoming[j];
            AudioNode *src = conn->source;
            
            if (!src->outputBuffer) {continue;}

            //maybe use SIMD here????
            for (int k = 0; k < numSamples; k++) {
                node->inputBuffer[k] += src->outputBuffer[k];
            }
        }

        node->interface->process(node->instance, 
                                 node->inputBuffer,
                                 node->outputBuffer,
                                 numSamples);

    }

    //free(processing_order);
}

static void free_node(AudioNode *node) {
    if (!node) {return;}

    if (node->interface && node->interface->destroy) {
        node->interface->destroy(node->instance);
    }
    free(node->inputBuffer);
    free(node->outputBuffer);
    free(node->incoming);
    free(node->outgoing);
}

static void free_connection(Connection *conn) {
    free(conn);
}

static AudioNode** topological_sort(AudioGraph *graph) {
    if (!graph || graph->numNodes == 0) {return NULL;}

    int *inDegree = (int*)calloc(graph->numNodes, sizeof(int));
    int *queue = (int*)malloc(graph->numNodes*sizeof(int));

    AudioNode** sorted = (AudioNode**)malloc(graph->numNodes*sizeof(AudioNode*));
    int front=0, rear=0, sortedIndex=0;

    if (!inDegree || !queue || !sorted ) {
        fprintf(stderr, "Failed to allocate memory in topological sort\n");
        free(inDegree);
        free(queue);
        free(sorted);
        return NULL;
    }

    for (int i=0; i < graph->numNodes; i++) {
        inDegree[i] = graph->nodes[i]->numIncoming;
    }

    for (int i=0; i<graph->numNodes; i++) {
        if (inDegree[i] == 0) {
            queue[rear++]=i;
        }
    }

    while (front < rear) {
        int current = queue[front++];
        sorted[sortedIndex++] = graph->nodes[current];

        for (int i=0; i<graph->nodes[current]->numOutgoing; i++) {
            Connection *conn = graph->nodes[current]->outgoing[i];

            int destIndex = -1;
            for (int j=0; j<graph->numNodes; i++) {
                if (graph->nodes[j] == conn->destination) {
                    destIndex = j;
                    break;
                }
            }

            if (destIndex != -1 && --inDegree[destIndex] == 0) {
                queue[rear++] = destIndex;
            }

        }
    }

    free(inDegree);
    free(queue);

    if (sortedIndex != graph->numNodes) {
        free(sorted);
        return NULL;
    }
    return sorted;
}
