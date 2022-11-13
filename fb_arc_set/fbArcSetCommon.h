#ifndef _FBARCSETCOMMONS_INCLUDED
#define _FBARCSETCOMMONS_INCLUDED 1

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <limits.h>
#include <time.h>

#define SHM_NAME "/fb_arc_set_shm_12123697"
#define SEM_FREE_NAME "/fb_arc_set_shm_12123697_SEM_FREE"
#define SEM_USED_NAME "/fb_arc_set_shm_12123697_SEM_USED"
#define SEM_BLOCKED_NAME "/fb_arc_set_shm_12123697_SEM_BLOCKED"
#define BUFFER_SIZE (16)

typedef struct edge {
    int node1;
    int node2;
} edge;

typedef struct solution {
    edge fbArcSet[8];
    int edgeCount;
} solution;

typedef struct sharedData {
    int state; // 0 => initializing 1 => ready 2 => terminating
    int writerPosition;

    solution buffer[BUFFER_SIZE];

} sharedData;

#endif