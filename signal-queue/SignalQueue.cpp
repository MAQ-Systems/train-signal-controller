/*
 * File: SignalQueue.cpp
 * Author: Matt Jones
 * Date: 5/4/2014
 * Desc: A simple worker-thread based message queue. All clients should
 *       remain connected as long as possible.
 * Protocol:
 *       Since there are a grand total of 8 possible things the signal
 *       can do, the message format will be simple:
 *       - One byte followed immediately by a null terminator ('\0')
 *       - Signal will operate based on which bits are set in the above byte
 *           - [0] blink flag
 *           - [1] red flag
 *           - [2] yellow flag
 *           - [3] green flag
 *           - [4] turn lamp on
 *           - [5] turn lamp off
 *           - [6] unused
 *           - [7] unused
 */

#include <queue>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <pthread.h>

using namespace std;


#define MAX_THREADS 3
#define MAX_QUEUE 50
#define SERVER_PORT "19100"
#define SERVER_BACKLOG 4


/**
 * ThreadInfo struct for managing threads
 */
typedef struct threadInfo {
    int workerId;   // ID of the thread
    bool connected; // If the worker is actuall in use
    int socketFd;   // The ID of the socket to write to
    bool queueWriter; // True if the client requests to write to the queue
} ThreadInfo;

ThreadInfo* threadInfoList;
pthread_mutex_t* threadMutexes; // used to stop and start particular threads (will block via spinlock or sleeping)
bool QUIT_FLAG;
queue<char*> signalQueue;


// prototypes
void* handleClient(void* param);
int findAvailableWorker(ThreadInfo* workers, const int noWorkers);


/**
 * Main method used to manage worker threads
 */
int main(int argc, char* argv[]) {

    int i;
    QUIT_FLAG = false;

    // thread vars
    threadMutexes = new pthread_mutex_t[MAX_THREADS];
    threadInfoList = new ThreadInfo[MAX_THREADS];
    pthread_t* threadIds = new pthread_t[MAX_THREADS];
    pthread_attr_t threadAttr;
    
    pthread_attr_init(&threadAttr);

    // initialize thread information & start threads
    for(i = 0; i < MAX_THREADS; i++) {
        threadInfoList[i].workerId = i;
        threadInfoList[i].connected = false;
        pthread_mutex_lock(&threadMutexes[i]);

        pthread_create(&threadIds[i], &threadAttr, handleClient, &threadInfoList[i]);
    }

    // socket vars
    struct sockaddr_storage clientAddr;
    socklen_t addrSize;
    struct addrinfo hints, *res;
    int sockFd, clientFd;

    // init socket info
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, SERVER_PORT, &hints, &res);
    sockFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if(sockFd < 0) {
        cout << "failed to bind to socket \n" << strerror(errno);
        exit(1);
    }

    bind(sockFd, res->ai_addr, res->ai_addrlen);
    listen(sockFd, SERVER_BACKLOG);

    // main loop waits for a new connection and assigns to a thread if one is available
    while(!QUIT_FLAG) {
        addrSize = sizeof(clientAddr);
        clientFd = accept(sockFd, (struct sockaddr *)&clientAddr, &addrSize);

        if(clientFd < 0) {
            cout << "Client did not connect successfully \n";
        }
        else {
            cout << "A client connected! \n";
        }

        // find available worker and assign
        int readyWorker = findAvailableWorker(threadInfoList, MAX_THREADS);
        if(readyWorker < 0) {
            // if we failed, send error message
            string errorMsg = "{\"error\":true,\"code\":1,\"message\":\"No available connections.\"}";
            //string errorMsg = "e:1";
            send(clientFd, errorMsg.c_str(), errorMsg.length(), 0);
            close(clientFd);
        }
        else {
cout << "accepted";
            // if we succeeded, send initiation message
            string welcomeMsg = "{\"error\":false}";
            //string errorMsg = "e:0";
            send(clientFd, welcomeMsg.c_str(), welcomeMsg.length(), 0);
            
            // set a thread as connected and unlock mutex for that thread
            threadInfoList[readyWorker].socketFd = clientFd;
            threadInfoList[readyWorker].connected = true;

            // finally unlock mutex so the thread starts going again
            pthread_mutex_unlock(&threadMutexes[readyWorker]);
        }
    }

    // join threads
    for(i = 0; i < MAX_THREADS; i++) {
        pthread_join(threadIds[i],NULL);
    }

    // clean up
    delete[] threadInfoList;
    delete[] threadIds;

    cout << "Server exiting...\n";

    return 0;
}



/**
 * Client handler that each thread will run
 */
void* handleClient(void* param) {

    int myId = ((ThreadInfo*)param)->workerId;
    
    // perpetually try to work
    while(!QUIT_FLAG) {
        if(threadInfoList[myId].connected) {

            // is the client going to be a reader or a writer?
            char cType;
            int size = recv(threadInfoList[myId].socketFd, &cType, 1, 0);

            if(cType == 'R' || cType == 'r') {
                // client is a reader 
                while(!signalQueue.empty()) {
                    if(signalQueue.size() < 50) {
                        char sig[3];
                        int size = recv(threadInfoList[myId].socketFd, &cType, 3, 0);
                        cout << sig;
                    }
                }
            }
            else if(cType == 'W' || cType == 'w') {
                // client is a writer
                int size;
                while(size > 0) {

                }
            }

            //stringstream msgStream;
            //msgStream << "hello from thread " << myId;
            //string msg = msgStream.str();

//            send(threadInfoList[myId].socketFd, msg.c_str(), msg.length(), 0);
            close(threadInfoList[myId].socketFd);
        }
    }

    cout << "Exiting thread " << myId << "\n";
}


/**
 * Find a worker thread that is not busy. If none are available, return -1.
 * @param workers A pointer to the array of worker information
 * @param noWorkers The number of workers available
 * @return The index of the next available worker or -1 if there are none
 */
int findAvailableWorker(ThreadInfo* workers, const int noWorkers) {
    if(workers == NULL) {
        return -1;
    }

    int i;
    for(i = 0; i < noWorkers; i++) {
        if(!workers[i].connected) {
            return i;
        }
    }

    return -1;
}

