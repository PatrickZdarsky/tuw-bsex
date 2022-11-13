#include "fbArcSetCommon.h"

static void writeError(char[], bool);
void handle_signal(int);
static int parseEdges(char*[], int, edge parsed[]);
static void shuffle(int[], int);
static int findFbArcSet(int, int[], int, edge[], edge[]);
static bool isInOrder(int, int, int[], int);
static void setup(void);
static void teardown(void);

char* programName;

int shmSetupState = 0;

int shmfd;
sharedData *data;

sem_t *semFree;
sem_t *semUsed;
sem_t *semBlocked;

/**
 * @brief The main entrypoint of the program
 * 
 * @param argc The amount of arguments which were passed to the program
 * @param argv The arguments which were passed to the program
 * @return int The program status code
 */
int main(int argc, char *argv[]) {
    programName = argv[0];

    if (argc == 1) {
        fprintf(stderr, "You have to supply at least one edge!\n");
        return EXIT_FAILURE;
    }

    int edgeCount = argc-1;
    edge edges[edgeCount];
    int nodeCount = parseEdges(argv, argc, edges);

    int nodes[nodeCount];
    for (int i = 0; i < nodeCount; i++)
    {
        nodes[i] = i;
    }

    //Initialize the pseudo-random number generator with seed
    srand(time(NULL) ^ getpid());

    setup();

    while (data -> state == 0) {
        //Wait for supervisor to become ready...
    }

    edge fbArcSet[edgeCount];
    int fbCount, bestFbCount = 100;
    while (data->state == 1) {
        shuffle(nodes, nodeCount);
        fbCount = findFbArcSet(nodeCount, nodes, edgeCount, edges, fbArcSet);

        //Check if our current solution is trash
        if (fbCount >= bestFbCount)
            continue;

        //We have found the best solution this generator has produced yet => post it to the supervisor
        bestFbCount = fbCount;

        printf("[%s] Got new solution with %d edges:", programName, fbCount);
        for (int i=0; i<fbCount; i++) {
            printf(" %d-%d", fbArcSet[i].node1, fbArcSet[i].node1);
        }
        printf("\n");

        if (sem_wait(semBlocked) == -1) {
            writeError("Error while 'blocked' semaphore is waiting", false);
            teardown();
            exit(EXIT_FAILURE);
        }

        //Check if we should shut down
        if (data->state != 1) {
            sem_post(semUsed);
            break;
        }

        if (sem_wait(semFree) == -1) {
            writeError("Error while 'free' semaphore is waiting", false);

            sem_post(semBlocked);
            sem_post(semUsed);

            if (errno != EINTR)
            {
                teardown();
                exit(EXIT_FAILURE);
            }
        }

        data->buffer[data->writerPosition].edgeCount = fbCount;
        for (int i=0; i<fbCount; i++) {
            data->buffer[data->writerPosition].fbArcSet[i].node1 = fbArcSet[i].node1;
            data->buffer[data->writerPosition].fbArcSet[i].node2 = fbArcSet[i].node2;
        }

        data->writerPosition = (data->writerPosition+1) % BUFFER_SIZE;

        sem_post(semUsed);
        sem_post(semBlocked);
    }

    teardown();

    return EXIT_SUCCESS;
}

/**
 * @brief Finds a new fb arc set for the given graph
 * 
 * @param nodeCount The amount of nodes in the graph
 * @param nodes The nodes, with the given oder
 * @param edgeCount The amount of edges in the graph
 * @param edges The edges of the graph
 * @param result An array where the resulting edges are stored
 * @return int The amount of edges which should be removed
 */
static int findFbArcSet(int nodeCount, int nodes[nodeCount], int edgeCount, edge edges[edgeCount], edge result[8]) {
    int fbCount = 0;

    for(int i=0; i<edgeCount; i++) {
        if (!isInOrder(edges[i].node1, edges[i].node2, nodes, nodeCount)) {
            result[fbCount].node1 = edges[i].node1;
            result[fbCount].node2 = edges[i].node2;

            fbCount++;
        }

        if (fbCount >= 8)
            break;
    }

    return fbCount;
}

/**
 * @brief Shuffle array using fischer-yates algorithm
 * 
 * @param nodes The array to shuffle
 * @param count The amount of elements in the array
 */
static void shuffle(int nodes[], int count) {
    int i, j, temp;

    for (i = count-1; i > 0; i--) {
        j = rand() % (i+1);
        
        //Swap
        temp = nodes[i];
        nodes[i] = nodes[j];
        nodes[j] = temp;
    }
}

/**
 * @brief Checks if the given two nodes are in order in the array
 * 
 * @param node1 The first node
 * @param node2 The second node
 * @param nodes The array with all nodes
 * @param nodeCount The amount of nodes which are stored in the array
 * @return true If the first node comes before the second
 * @return false If the first node comes after the second
 */
static bool isInOrder(int node1, int node2, int nodes[], int nodeCount) {
    int index1 = -1, index2 = -1;

    for (int i=0; i<nodeCount; i++) {
        if (nodes[i] == node1)
            index1 = i;
        else if (nodes[i] == node2)
            index2 = i;

        if (index1 != -1 && index2 != -1)
            break;
    }

    return index1 < index2;
}

/**
 * @brief Parses the supplied edges directly from program arguments
 * 
 * @param inputEdges The program arguments array
 * @param count The number of arguments in the array
 * @param parsed The array, where the parsed edges will be saved
 * @return int The id of the node with the highest value
 */
static int parseEdges(char *inputEdges[], int count, edge parsed[count-1]) {
    int highestNode = 0;

    for (int i=1; i<count; i++) {
        int edgeIndex = i-1;
        char *endptr = NULL; 
        errno = 0; // To distinguish success/failure after call

        long node1 = strtol(inputEdges[i], &endptr, 10);
        if (inputEdges[i] == endptr || ((node1 == LONG_MIN || node1 == LONG_MAX) && errno == ERANGE)) {
            writeError("Could not parse supplied edges", false);
            exit(EXIT_FAILURE);
        }
        parsed[edgeIndex].node1 = (int) node1;

        
        char *sndNodePtr = strstr(inputEdges[i], "-");
        if (sndNodePtr == NULL) {
            writeError("Could not parse supplied edges", false);
            exit(EXIT_FAILURE);
        }
        sndNodePtr += 1; //skip '-'

        errno = 0;
        endptr = NULL;

        long node2 = strtol(sndNodePtr, &endptr, 10);
        if (sndNodePtr == endptr || ((node2 == LONG_MIN || node2 == LONG_MAX) && errno == ERANGE)) {
            writeError("Could not parse supplied edges", false);
            exit(EXIT_FAILURE);
        }
        parsed[edgeIndex].node2 = (int) node2;

        if (parsed[edgeIndex].node1 > highestNode)
            highestNode = parsed[edgeIndex].node1;
        if (parsed[edgeIndex].node2 > highestNode)
            highestNode = parsed[edgeIndex].node2;
    }

    return highestNode+1;
}

/**
 * @brief Handles any signals from the operating system
 * 
 * @param signal The signal which was sent to the program
 */
void handle_signal(int signal) { 
    if (shmSetupState == 3) {
        data->state = 2;
        teardown();
    }
}

/**
 * @brief Sets the shared memory up and the semaphores
 * 
 */
static void setup() {
    shmfd = shm_open(SHM_NAME, O_RDWR, 0600);

    if (shmfd == -1) {
        writeError("Could not open shared memory", true);
        exit(1);
    }
    shmSetupState = 2;

    data = mmap(NULL, sizeof(*data), PROT_READ | PROT_WRITE, 
                        MAP_SHARED, shmfd, 0);

    if (data == MAP_FAILED) {
        writeError("Could not map shared memory", true);
        exit(1);
    }
    shmSetupState = 3;

    if ((semFree = sem_open(SEM_FREE_NAME, 0)) == SEM_FAILED) {
        writeError("Could not open 'free' semaphore", true);
        exit(1);
    }
    if ((semUsed = sem_open(SEM_USED_NAME, 0)) == SEM_FAILED) {
        writeError("Could not open 'used' semaphore", true);
        exit(1);
    }
    if ((semBlocked = sem_open(SEM_BLOCKED_NAME, 0)) == SEM_FAILED) {
        writeError("Could not open 'blocked' semaphore", true);
        exit(1);
    }
}

/**
 * @brief Properly closes the shared memory and the semaphores
 * 
 */
static void teardown() {

    switch (shmSetupState) {
        case 3:
        case 2:
            if (munmap(data, sizeof(*data)) == -1) {
                writeError("Could not unmap shared memory", true);
            }
        case 1:
            if (close(shmfd) == -1) {
                writeError("Could not close shared memory", true);
            }
    }   

    //Close semaphores last, if they were set up
    if (shmSetupState == 3) {
        if (sem_close(semFree) != 0) {
            writeError("Could not close 'free' semaphore", true);
        }
        if (sem_close(semUsed) != 0) {
            writeError("Could not close 'used' semaphore", true);
        }
        if (sem_close(semBlocked) != 0) {
            writeError("Could not close 'blocked' semaphore", true);
        }
    }
}

/**
 * @brief Writes an error to the stderr output
 * 
 * @param errMessage The message to describe the error
 * @param errnoUsed If the errno variable should be printed
 */
static void writeError(char errMessage[], bool errnoUsed) {
    fprintf(stderr, "[%s] %s", programName, errMessage);
    if (errnoUsed)
        fprintf(stderr, ": %s", strerror(errno));
    fprintf(stderr, "\n");
}