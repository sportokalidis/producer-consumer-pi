#include "timer.h"

Timer *timerInit(int period, int tasksToExecute, int startDelay , queue *queue, void *(*producer)(void *arg),void *(*errorFnc)(), void * (*timerFnc) (void *)){
    printf("Initializing Timer\n");
    Timer *T = (Timer *) malloc( sizeof(Timer) );
    T->period = period;
    T->tasksToExecute = tasksToExecute;
    T->startDelay = startDelay;
    T->q = queue;
    T->producer = producer;
    T->errorFnc = errorFnc;
    T->timerFnc = timerFnc;
    T->startFnc = NULL;
    T->userData = NULL;
    // na valw tid

    return T;
}

void start(Timer *T){
 pthread_create(&T->tid, NULL, T->producer, T);
}

void startat(Timer *T, int year, int month, int day, int hour, int minute, int second){

}


void timerStop(Timer *T){
  free( T->startFnc );
  free( T->timerFnc );
  free( T->stopFnc );
  free( T->errorFnc );
  free( T );
}

void *error(int* jobsLostCounter) {
    *jobsLostCounter = *jobsLostCounter + 1;

    return(NULL);
}
