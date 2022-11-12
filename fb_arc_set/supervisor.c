#include "fbArcSetCommon.h"

static void setup();
static void teardown();
void handle_signal(int);

int shmfd;
sharedData *data;

sem_t *semFree;
sem_t *semUsed;
sem_t *semBlocked;

int main(int argc, char *argv[]) {
    printf("Supervisor\n");

    // setup signal handler
    struct sigaction sa = {.sa_handler = handle_signal};
    if (sigaction(SIGINT, &sa, NULL) + sigaction(SIGTERM, &sa, NULL) < 0)
    {
        printf("Error while initializing signal handler %d\n", errno);
        exit(EXIT_FAILURE);
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
                fprintf(stderr, "Error while semaphore is waiting\n");
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
            printf("The graph is acyclic!\n");
            //Todo: Set semaphore for each generator to terminate?
            data->state = 2;
            break;
        }

        if (sol.edgeCount < currentBestEdgeCount) {
            currentBestEdgeCount = sol.edgeCount;

            printf("New solution with %d edges:", currentBestEdgeCount);
            for (int i=0; i<sol.edgeCount; i++) {
                printf(" %d-%d", sol.fbArcSet[i].node1, sol.fbArcSet[i].node2);
            }
            printf("\n");
        }

        sem_post(semFree);
    }

    printf("Shutting down...\n");
    teardown();
}

void handle_signal(int signal) { data->state = 2; }

static void setup() {
    shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0600);

    if (shmfd == -1) {
        fprintf(stderr, "Could not open shared memory: %s\n", strerror(errno));
        exit(1);
    }

    if (ftruncate(shmfd, sizeof(sharedData)) == -1) {
        fprintf(stderr, "Could not truncate shared memory: %s\n", strerror(errno));
        exit(1);
    }

    data = mmap(NULL, sizeof(*data), PROT_READ | PROT_WRITE, 
                        MAP_SHARED, shmfd, 0);

    if (data == MAP_FAILED) {
        fprintf(stderr, "Could not map shared memory: %s\n", strerror(errno));
        exit(1);
    }

    //Setup semaphores
    if ((semFree = sem_open(SEM_FREE_NAME, O_CREAT | O_EXCL, 0600, BUFFER_SIZE)) == SEM_FAILED) {
        fprintf(stderr, "Could not open free semaphore: %s\n", strerror(errno));
        exit(1);
    }
    if ((semUsed = sem_open(SEM_USED_NAME, O_CREAT | O_EXCL, 0600, 0)) == SEM_FAILED){
        fprintf(stderr, "Could not open used semaphore: %s\n", strerror(errno));
        exit(1);
    }
    if ((semBlocked = sem_open(SEM_BLOCKED_NAME, O_CREAT | O_EXCL, 0600, 1)) == SEM_FAILED){
        fprintf(stderr, "Could not open blocked semaphore: %s\n", strerror(errno));
        exit(1);
    }
    
}

static void teardown() {
    if (sem_close(semFree) != 0) {
        fprintf(stderr, "Could not close free semaphore: %s\n", strerror(errno));
        exit(1);
    }
    if (sem_close(semUsed) != 0) {
        fprintf(stderr, "Could not close used semaphore: %s\n", strerror(errno));
        exit(1);
    }
    if (sem_close(semBlocked) != 0) {
        fprintf(stderr, "Could not close blocked semaphore: %s\n", strerror(errno));
        exit(1);
    }

    if (sem_unlink(SEM_FREE_NAME) != 0) {
        fprintf(stderr, "Could not unlink free semaphore: %s\n", strerror(errno));
        exit(1);
    }
    if (sem_unlink(SEM_USED_NAME) != 0) {
        fprintf(stderr, "Could not unlink used semaphore: %s\n", strerror(errno));
        exit(1);
    }
    if (sem_unlink(SEM_BLOCKED_NAME) != 0) {
        fprintf(stderr, "Could not unlink blocked semaphore: %s\n", strerror(errno));
        exit(1);
    }    


    if (munmap(data, sizeof(*data)) == -1) {
        fprintf(stderr, "Could not unmap shared memory: %s\n", strerror(errno));
        exit(1);
    }
    if (close(shmfd) == -1) {
        fprintf(stderr, "Could not close shared memory: %s\n", strerror(errno));
        exit(1);
    }
    if (shm_unlink("/fb_arc_set_12123697") != 0) {
        fprintf(stderr, "Could not unlink shared memory: %s\n", strerror(errno));
        exit(1);
    }

    //Todo: Cleanup semaphores
}