#include "fbArcSetCommon.h"

static void writeError(char[], bool);
static void setup();
static void teardown();
void handle_signal(int);

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

    if (argc > 1) {
        fprintf(stderr, "[%s] You must not supply any arguments\n", programName);
        return EXIT_FAILURE;
    }

    // setup signal handler
    struct sigaction sa = {.sa_handler = handle_signal};
    if (sigaction(SIGINT, &sa, NULL) + sigaction(SIGTERM, &sa, NULL) < 0)
    {
        writeError("Error while initializing signal handler\n", true);
        return EXIT_FAILURE;
    }

    setup();
    
    //Main Supervisor logic

    int readerPosition = 0;
    int currentBestEdgeCount = 9;

    data->writerPosition = 0;
    data->state = 1;

    while (data->state == 1) {
        //Todo: check for state again??

        if (sem_wait(semUsed) == -1)
        {
            if (errno != EINTR)
            {
                writeError("Error while 'used' semaphore is waiting", false);
                teardown();
                exit(EXIT_FAILURE);
            }

            // => interrupted by SIGINT
            //Todo: Set semaphore for each generator to terminate?
            data->state = 2;
            break;
        }

        solution sol = data->buffer[readerPosition];
        readerPosition = (readerPosition + 1) % BUFFER_SIZE;

        if (sol.edgeCount == 0) {
            printf("[%s] The graph is acyclic!\n", programName);
            //Todo: Set semaphore for each generator to terminate?
            data->state = 2;
            break;
        }

        if (sol.edgeCount < currentBestEdgeCount) {
            currentBestEdgeCount = sol.edgeCount;

            printf("[%s] New solution with %d edges:", programName, currentBestEdgeCount);
            for (int i=0; i<sol.edgeCount; i++) {
                printf(" %d-%d", sol.fbArcSet[i].node1, sol.fbArcSet[i].node2);
            }
            printf("\n");
        }

        sem_post(semFree);
    }

    teardown();
}

/**
 * @brief Handles any signals from the operating system
 * 
 * @param signal The signal which was sent to the program
 */
void handle_signal(int signal) { 
    data->state = 2; 
}

/**
 * @brief Sets the shared memory up and the semaphores
 * 
 */
static void setup() {
    shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0600);

    if (shmfd == -1) {
        writeError("Could not open shared memory", true);
        teardown();
        exit(1);
    }
    shmSetupState = 1;

    if (ftruncate(shmfd, sizeof(sharedData)) == -1) {
        writeError("Could not truncate shared memory", true);
        teardown();
        exit(1);
    }
    shmSetupState = 2;

    data = mmap(NULL, sizeof(*data), PROT_READ | PROT_WRITE, 
                        MAP_SHARED, shmfd, 0);

    if (data == MAP_FAILED) {
        writeError("Could not map shared memory", true);
        teardown();
        exit(1);
    }
    shmSetupState = 3;

    if ((semFree = sem_open(SEM_FREE_NAME, O_CREAT | O_EXCL, 0600, BUFFER_SIZE)) == SEM_FAILED) {
        writeError("Could not open 'free' semaphore", true);
        teardown();
        exit(EXIT_FAILURE);
    }
    if ((semUsed = sem_open(SEM_USED_NAME, O_CREAT | O_EXCL, 0600, 0)) == SEM_FAILED){
        writeError("Could not open 'used' semaphore", true);
        teardown();
        exit(EXIT_FAILURE);
    }
    if ((semBlocked = sem_open(SEM_BLOCKED_NAME, O_CREAT | O_EXCL, 0600, 1)) == SEM_FAILED){
        writeError("Could not open 'blocked' semaphore", true);
        teardown();
        exit(EXIT_FAILURE);
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
            if (shm_unlink(SHM_NAME) == -1) {
                writeError("Could not unlink shared memory", true);
            }
    }  

    //Teardown semaphores last, if they were set up
    if (shmSetupState == 3) {
        if (sem_close(semFree) == -1) {
            writeError("Could not close 'free' semaphore", true);
        } else if (sem_unlink(SEM_FREE_NAME) == -1) {
                writeError("Could not unlink 'free' semaphore", true);
        }
        if (sem_close(semUsed) == -1) {
            writeError("Could not close 'used' semaphore", true);
        } else if (sem_unlink(SEM_USED_NAME) == -1) {
                writeError("Could not unlink 'used' semaphore", true);
        }
        if (sem_close(semBlocked) == -1) {
            writeError("Could not close 'blocked' semaphore", true);
        } else if (sem_unlink(SEM_BLOCKED_NAME) == -1) {
                writeError("Could not unlink 'blocked' semaphore", true);
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