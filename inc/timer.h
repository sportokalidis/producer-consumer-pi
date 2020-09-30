
#ifndef TIMER_H
#define TIMER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include "queue.h"

typedef struct{
    int period;
    int tasksToExecute;
    int startDelay;

    void *(*startFnc)(void *);
    void *(*stopFnc)(void *);
    void *(*timerFnc)(void *);
    void *(*errorFnc)(void *);
    void *userData;
    queue *q;
    void *(*producer)(void *arg);
    pthread_t tid;
} Timer;


Timer *timerInit(int period, int tasksToExecute, int startDelay , queue *queue, void *(*producer)(void *arg),void *(*errorFnc)(), void * (*timerFnc) (void *););

void timerStop(Timer *T);

void start(Timer *T);

void startat(Timer *T, int year, int month, int day, int hour, int minute, int second);

void *error(int *jobsLostCounter);


#endif
