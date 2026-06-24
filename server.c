#include  "stdbool.h"
#include "segel.h"
#include "request.h"
//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// HW3: Parse the new arguments too
typedef struct Queue *Queue;
typedef struct Node *Node;


///Relevant Queues
Queue runningQueue = NULL;
Queue waitingQueue= NULL;
Queue realTimeQueue= NULL;

///Conditions
pthread_cond_t notEmptyQueue;
pthread_cond_t availableThreads;
pthread_cond_t allThreadsAvailable;
pthread_cond_t noVIP;
pthread_cond_t VIP;
pthread_mutex_t theMutex;

int VIPrunning;

///The threads function
int handleOverload(int connfd,char* policy,int isRealTime,struct timeval arrival,int maxrequests);
void* normalThreads(void* args);
void* vipThread(void* args);
void getargs(int *port, int *threads_num, int *queue_size, int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *threads_num = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
    if(*port <= 1024 || *threads_num <= 0 || *queue_size <= 0)
	exit(1);

}


int main(int argc, char *argv[]) {
    int listenfd, connfd, port, maxthreads, maxrequests, clientlen;
    struct sockaddr_in clientaddr;
    struct timeval arrivalTime;
    char *policy = strdup(argv[4]); 
	if(!(strcmp(policy, "dt") == 0 || strcmp(policy, "block") == 0 || strcmp(policy, "dh") == 0 || strcmp(policy, "bf") == 0 || strcmp(policy, "random") == 0))
		exit(1);
    getargs(&port, &maxthreads, &maxrequests, argc, argv);
    waitingQueue = initializeQueue();
    runningQueue = initializeQueue();
    realTimeQueue = initializeQueue();
    pthread_cond_init(&notEmptyQueue, NULL);
    pthread_cond_init(&availableThreads, NULL);
    pthread_cond_init(&noVIP, NULL);
    pthread_cond_init(&allThreadsAvailable, NULL);
    pthread_mutex_init(&theMutex, NULL);
    pthread_t threads[maxthreads + 1];
    VIPrunning = 0;
    // Create worker threads
    for (int i = 0; i < maxthreads; i++) {
        tStats curr = statsCons(i);
        pthread_create(&threads[i], NULL, normalThreads, (void *)curr);
    }

    // Create VIP thread
    tStats curr = statsCons(maxthreads);
    pthread_create(&threads[maxthreads], NULL, vipThread, (void *)curr);

    listenfd = Open_listenfd(port);
    while (1) {
        int ret = 0;
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);
        int isRealTime = getRequestMetaData(connfd);
        gettimeofday(&arrivalTime, NULL);
        pthread_mutex_lock(&theMutex);

        if (getSize(runningQueue) + getSize(waitingQueue) + getSize(realTimeQueue) == maxrequests) {
            ret = handleOverload(connfd, policy, isRealTime, arrivalTime, maxrequests);
        }
        if(!ret) {
            Node newNode = createNode(connfd, arrivalTime);
            if (!isRealTime) {
                addWaitingNode(waitingQueue, newNode);
                pthread_cond_signal(&notEmptyQueue);
            } else {
                addWaitingNode(realTimeQueue, newNode);
                pthread_cond_signal(&VIP);
            }
        }
        pthread_mutex_unlock(&theMutex);
    }
    free(policy);  // Free allocated memory for policy
    return 0;
}

void* normalThreads(void* args)
{
    while (1) {
        pthread_mutex_lock(&theMutex);
        while (getSize(waitingQueue) == 0) {
            pthread_cond_wait(&notEmptyQueue, &theMutex);
        }
        while (getSize(realTimeQueue) > 0 || VIPrunning == 1) {
            pthread_cond_wait(&noVIP, &theMutex);
        }
        tStats stats = (tStats)args;
        Node front = popFront(waitingQueue);
        addWorkingNode(runningQueue, front, stats->m_ID);
        pthread_mutex_unlock(&theMutex);
        if(requestHandle(getValue(front), args,front) == 0) {
            Close(getValue(front));
            pthread_mutex_lock(&theMutex);
            removeByFD(runningQueue, getValue(front));
	     pthread_cond_signal(&availableThreads);
            if (getSize(realTimeQueue) + getSize(waitingQueue) + getSize(runningQueue) == 0) {
                pthread_cond_signal(&allThreadsAvailable);
            }
            pthread_mutex_unlock(&theMutex);
        }
    }
}
void* vipThread(void* args)
{
    while (1) {
        pthread_mutex_lock(&theMutex);
        while (getSize(realTimeQueue) == 0) {
            pthread_cond_wait(&VIP, &theMutex);
        }
        tStats stats = (tStats)args;
        Node front = popFront(realTimeQueue);
        addWorkingNode(runningQueue, front, stats->m_ID);
	VIPrunning = 1;
        pthread_mutex_unlock(&theMutex);
        requestHandle(getValue(front), args,front);
        Close(getValue(front));
        pthread_mutex_lock(&theMutex);
        removeByFD(runningQueue, getValue(front));
	VIPrunning = 0;
        pthread_cond_signal(&availableThreads);
        if (getSize(realTimeQueue) == 0) {
            pthread_cond_signal(&noVIP);
        }
	pthread_cond_signal(&availableThreads);
        if (getSize(realTimeQueue)+getSize(waitingQueue) + getSize(runningQueue) == 0) {
            pthread_cond_signal(&allThreadsAvailable);
        }
        pthread_mutex_unlock(&theMutex);
    }
}
///return 1 if we don't want to add upon return
int handleOverload(int connfd, char *policy, int isRealTime, struct timeval arrival, int maxrequests) {
    if (strcmp(policy, "block") == 0 || getSize(waitingQueue) == 0) {
        while (getSize(runningQueue) + getSize(waitingQueue) + getSize(realTimeQueue) == maxrequests) {
            pthread_cond_wait(&availableThreads, &theMutex);
        }
    } else if (strcmp(policy, "dt") == 0) {
        if (!isRealTime) {
            Close(connfd);
            return 1;
        }
        int fd = popBack(waitingQueue)->m_value;
        Close(fd);
    } else if (strcmp(policy, "random") == 0) {
        int drop_number = (getSize(waitingQueue) + 1) / 2;
        for (int i = 0; i < drop_number; i++) {
            int indexToDrop = rand() % getSize(waitingQueue);
            int fd = removeByIndex(waitingQueue, indexToDrop);
            Close(fd);
        }
    } else if (strcmp(policy, "dh") == 0) {
        int fd = popFront(waitingQueue)->m_value;
        Close(fd);
    } else if (strcmp(policy, "bf") == 0) {
        while (getSize(realTimeQueue) + getSize(waitingQueue) + getSize(runningQueue) > 0) {
            pthread_cond_wait(&allThreadsAvailable, &theMutex);
        }
        if (!isRealTime) {
            Close(connfd);
            return 1;
        }
    }
    return 0;
}