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
#include <pthread.h>

using namespace std;


#define MAX_THREADS 4


void* handleClient(void* param);


/**
 * ThreadInfo struct for managing threads
 */
typedef struct threadInfo {
    int workerId;
    bool connected;
} ThreadInfo;

ThreadInfo* threadInfoList;
bool quitFlag;

/**
 * Main method used to manage worker threads
 */
int main(int argc, char* argv[]) {

    int i;
    threadInfoList = new ThreadInfo[MAX_THREADS];
    pthread_t* threadIds = new pthread_t[MAX_THREADS];
    pthread_attr_t threadAttr;
    
    quitFlag = false;

    // initialize thread information & start threads
    for(i = 0; i < MAX_THREADS; i++) {
        threadInfoList[i].workerId = i;
        threadInfoList[i].connected = false;

        pthread_create(&threadIds[i], &threadAttr, handleClient, &threadInfoList[i]);
    }

    // main loop waits for a new connection and assigns to a thread if one is available
    //while(!quitFlag) {

    //}

    // join threads
    for(i = 0; i < MAX_THREADS; i++) {
        pthread_join(threadIds[i],NULL);
    }

    // clean up
    delete[] threadInfoList;
    delete[] threadIds;

    return 0;
}



/**
 * Client handler that each thread will run
 */
void* handleClient(void* param) {

    int myId = 0;//((ThreadInfo*)param)->workerId;

    cout << "in thread " << myId << "\n";
}

