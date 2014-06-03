/*
 * File: SignalQueue.cpp
 * Author: Matt Jones
 * Date: 5/4/2014
 * Desc: A simple worker-thread based message queue. All clients should
 *       remain connected as long as possible.
 * Protocol:
 *       Since there are a grand total of 8 possible things the signal
 *       can do, the message format will be simple:
 *       - R/W/S/X to specify client type (read, write, server, none)
 *       - '|' delimiter
 *       - T/F to indicate error
 *       - '|' delimiter
 *       - One byte for state - based on which bits are set in this byte
 *           - [0] ALWAYS SET TO 1
 *           - [1] blink flag
 *           - [2] red flag
 *           - [3] yellow flag
 *           - [4] green flag
 *           - [5] turn lamp on
 *           - [6] turn lamp off
 *           - [7] unused
 *       - '\0' to close the message
 * Reserved Chars:
 *      '[', ']', '|', '\0'
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
    pthread_mutex_t threadMutex; // used to stop and start particular a thread (will block via spinlock or sleeping)
} ThreadInfo;

/**
 * SignalMessage struct for parsing signal messages
 * NOTE: See comments at top of file for documentation on these fields
 */
typedef struct signalMessage {
    char clientType;
    bool error;
    char signalState;
} SignalMessage;

bool QUIT_FLAG;
int* LISTENING_SOCKET;
queue<char*> signalQueue;
pthread_mutex_t signalQueueMutex;


// prototypes
void* handleClient(void* param);
void handleReader(ThreadInfo* tInfo);
void handleWriter(ThreadInfo* tInfo);
int findAvailableWorker(ThreadInfo* workers, const int noWorkers);
bool isValidMessage(char* msg, int len);
SignalMessage* parseSignalMessage(char* msg, int len);


/**
 * Main method used to manage worker threads
 */
int main(int argc, char* argv[]) {

    // TODO: Remove this test data
    char* temp = new char[10];
    char data[] = "TESTDATA\0";
    memcpy(temp,data,strlen(data)+1);
    signalQueue.push(temp);


    int i;
    QUIT_FLAG = false;

    // thread vars
    ThreadInfo* threadInfoList = new ThreadInfo[MAX_THREADS];
    pthread_t* threadIds = new pthread_t[MAX_THREADS];
    pthread_mutex_init(&signalQueueMutex,NULL);
    
    // initialize thread information & start threads
    for(i = 0; i < MAX_THREADS; i++) {
        threadInfoList[i].workerId = i;
        threadInfoList[i].connected = false;
        
        pthread_mutex_init(&threadInfoList[i].threadMutex,NULL);
        pthread_mutex_lock(&threadInfoList[i].threadMutex);

        // using NULL for pthread_attr_t (just use default behavior)
        pthread_create(&threadIds[i], NULL, handleClient, &threadInfoList[i]);
    }

    // socket vars
    struct sockaddr_storage clientAddr;
    socklen_t addrSize;
    struct addrinfo hints, *res;
    int sockFd, clientFd;

    // init socket info
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    // TODO: possibly set socket option SO_REUSEADDR so socket is available sooner (close does not immediately clean up)
    getaddrinfo(NULL, SERVER_PORT, &hints, &res);
    sockFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    LISTENING_SOCKET = &sockFd;

    if(sockFd < 0) {
        cout << "Failed to create listening socket: \n" << strerror(errno);
        exit(1);
    }

    if(bind(sockFd, res->ai_addr, res->ai_addrlen) < 0) {
        cout << "Failed to bind address to socket: \n" << strerror(errno);
        exit(1);
    }

    if(listen(sockFd, SERVER_BACKLOG) < 0) {
        cout << "Failed to listen on socket: \n" << strerror(errno);
        exit(1);
    }

    addrSize = sizeof(clientAddr);

    // main loop waits for a new connection and assigns to a thread if one is available
    while(!QUIT_FLAG) {
        clientFd = accept(sockFd, (struct sockaddr *)&clientAddr, &addrSize);
        
        // the loop will likely continue once before the flag is set
        if(QUIT_FLAG) {
            break;
        }

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
            //string errorMsg = "{\"error\":true,\"code\":1,\"message\":\"No available connections.\"}";
            string errorMsg = "S|T|1";
            send(clientFd, errorMsg.c_str(), errorMsg.length()+1, 0);
            close(clientFd);
        }
        else {
            // if we succeeded, send initiation message
            //string welcomeMsg = "{\"error\":false}";
            string welcomeMsg = "S|F|1";
            send(clientFd, welcomeMsg.c_str(), welcomeMsg.length()+1, 0);
            
            // set a thread as connected and unlock mutex for that thread
            threadInfoList[readyWorker].socketFd = clientFd;
            threadInfoList[readyWorker].connected = true;

            // finally unlock mutex so the thread starts going again
            pthread_mutex_unlock(&threadInfoList[readyWorker].threadMutex);
        }
    }

    // clean up everything
    cout << "Waiting for threads to exit...\n";
    cout.flush();

    // join threads
    for(i = 0; i < MAX_THREADS; i++) {
        // unlock each thread's mutex
        pthread_mutex_unlock(&threadInfoList[i].threadMutex);
        // close thread's connection
        if(threadInfoList[i].connected) {
            close(threadInfoList[i].socketFd);
        }
        // wait for thread to exit
        pthread_join(threadIds[i],NULL);
    }

    // empty out queue
    char* tempSig;
    while(!signalQueue.empty()) {
        tempSig = signalQueue.front();
        signalQueue.pop();
        if(tempSig != NULL) {
            delete tempSig;
        }
    }

    // close listening socket
    close(sockFd); // this should already be closed, but just in case...

    delete[] threadInfoList;
    delete[] threadIds;

    cout << "Server exiting...\n";

    return 0;
}



/**
 * Client handler that each thread will run
 */
void* handleClient(void* param) {

    // keep handle to thread info (this will be changed by master thread)
    ThreadInfo* tInfo = (ThreadInfo*)param;   

    int myId = tInfo->workerId;
 
    // perpetually try to work
    while(!QUIT_FLAG) {

        // wait for master to unlock this thread
        pthread_mutex_lock(&tInfo->threadMutex);

        cout << "Client connected to thread " << myId << "\n";
        int clientSoc = tInfo->socketFd;

        if(tInfo->connected) {

            // is the client going to be a reader or a writer?
            char* sig = new char[32];
            int size = recv(clientSoc, sig, 32, 0);
            if(size < 1) {
                cout << "Client disconnected before any useful information was sent.\n";
            }
            else {
                SignalMessage* msg = parseSignalMessage(sig, size);

                if(msg == NULL) {
                    cout << "Parsed message was NULL.\n";
                }
                else {
                    char clientType = msg->clientType;

                    cout << "Clien Type: " << msg->clientType << "\n";
                    cout << "Error: " << msg->error << "\n";
                    cout << "Signal State: " << msg->signalState << "\n";

                    // clean up one-time use info
                    delete msg;
                    delete sig;

                    switch(clientType) {
                        case 'R':
                        case 'r':
                            // client is a reader
                            handleReader(tInfo);
                            break;
                        case 'W':
                        case 'w':
                            // client is a writer
                            handleWriter(tInfo);
                            break;
                        case 'X':
                        case 'x':
                            QUIT_FLAG = true;
                            if(close(*LISTENING_SOCKET) < 0) {
                                cout << "failed to close listening socket!\n";
                                cout.flush();
                            }
                            break;
                    }
                }
            }

            cout << "closing socket\n";
            close(clientSoc);
        }
        
        tInfo->connected = false;
    }

    cout << "Exiting thread " << myId << "\n";
    cout.flush();
}

/**
 * Handle a client that wants to read from the queue
 * @param tInfo A pointer to the thread's ThreadInfo struct
 */
void handleReader(ThreadInfo* tInfo) {
    char* sig = NULL;
    int size = 0; 
    int clientSoc = tInfo->socketFd;
    while(!signalQueue.empty()) {
        // make sure I am the only thread modifying the queue
        pthread_mutex_lock(&signalQueueMutex);
        sig = signalQueue.front();
        signalQueue.pop();
        // done modifying the queue
        pthread_mutex_unlock(&signalQueueMutex);
        size = send(clientSoc, sig, strlen(sig)+1, 0);
cout << "MESSAGE: " << sig << "\n";
        delete[] sig;

        if(size < 1) {
            cout << "Client disconnected!\n";
            break;
        }
    }
}

/**
 * Handle a client that wants to write to the queue
 * @param tInfo A pointer to the thread's ThreadInfo struct
 */
void handleWriter(ThreadInfo* tInfo) {
    char* sig = NULL; 
    int size = 0; 
    int clientSoc = tInfo->socketFd;
    int msgCount = 0;
    while(tInfo->connected) {
        sig = new char[32];
        size = recv(clientSoc, sig, 31, 0); // leave space for null terminator
        if(size > 0) {
            sig[size+1] = '\0';
        }
        else {
            cout << "Client disconnected!\n";
            break;
        }

        // make sure I am the only thread modifying the queue
        pthread_mutex_lock(&signalQueueMutex);

        if(signalQueue.size() < 50 && isValidMessage(sig,size)) {
            msgCount++;
            signalQueue.push(sig);
cout << "MESSAGE: " << sig << "\nSIZE: " << size << "\n";
        } else {cout << "no msg.......\n";}
        // done modifying the queue
        pthread_mutex_unlock(&signalQueueMutex);
    }
cout << "Received: " << msgCount << "\n";
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

/**
 * Determine if a given message loosely follows protocol
 * @param msg The character array containing the message from a client
 *            trying to write to the queue.
 * @param len The length of the message
 */
bool isValidMessage(char* msg, int len) {
    if(msg == NULL) {
        return false;
    }

    // make sure message is not too large
    if(len > 32) {
        return false;
    }

    // make sure message closing character is present
    if(msg[len-1] != '\0') {
        return false;
    }

    return true;
}

/**
 * Parse message into SignalMessage struct
 * @param msg The char array pointer from the client
 * @param len The length of the message
 * @return Pointer to a new SignalMessage
 */
SignalMessage* parseSignalMessage(char* msg, int len) {
    if(!isValidMessage(msg, len)) {
        return NULL;
    }

    SignalMessage* sm = new SignalMessage;

    // init signal message
    sm->clientType = 'X';
    sm->error = false;
    sm->signalState = 1;

    char *tokSave;
    char *token = strtok_r(msg, "|", &tokSave);
    int elementNo = 0;

    // fill struct by stepping through tokens (easy to keep default settings)
    while(token != NULL) {
        if(strlen(token) > 0) {
            switch(elementNo) {
                case 0:
                    sm->clientType = token[0];
                    break;

                case 1:
                    sm->error = token[0]=='T'?true:false;
                    break;

                case 2:
                    sm->signalState = token[0];
                    break;
            }
        }
        elementNo++;
        token = strtok_r(NULL, "|", &tokSave);
    }

    return sm;
}

