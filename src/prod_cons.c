/*
 *	File	: prod_cons.c
 *
 *	Title	: Implementations of producers and consumers functions
 *
 *	Short	: A solution to the producer consumer problem using pthreads.
 *
 *
 *	Author	: Portokalidis Stavros
 *
 *	Date	: 25 March 2020
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <timer.h>

#include "queue.h"
#include "functions.h"



#define NUM_OF_FUNCTIONS 10    // Number of functions that we will use
// #define LOOP 300000            // The number of objects that a producer add to queue's buffer
#define P 1                    // Number of producers
#define Q 2                    // Number of consumers

#define MODE 1
#define RUN_TIME 3600            // in seconds


#define NUM_OF_ARGS 30
// Array with possible workFunctions args
int arguments[NUM_OF_ARGS] = { 2, 3, 4, 5, 7, 8, 10, 11, 13, 14, 16, 18,
                               21, 22, 24, 26, 28, 31, 32, 35, 40,
                               45, 50, 60, 70, 80, 85, 90, 95, 100 };

int counter = 0;               // counter: count the number of producer's threads that finish
// int remaining_time_counter=0;  //
int flag=0;                    // flag that inform us if all producer's threads had finished


typedef struct{
 queue *q;
 pthread_mutex_t *consumerMut;
} consumerArg;

typedef struct{
  int *buffer;
  int index;
} metrixArray;


// Global flags/counters/times
int prodFinished = 0;
int terminateConFlag = 0;
struct timeval timeStamp;
double tempTime = 0;
int  jobsLostCounter = 0;

struct timeval arrTime[QUEUESIZE];


FILE *f1;
FILE *f2;
FILE *f3;
FILE *f4;


 metrixArray *metrixArrayInit(int n);

 void metrixArrayAdd(FILE *file_to_write , int value);

// implementation of producer
/*
 * parameters:
 * q -> pointer to queue
*/
void *producer (void *q)
{
  queue *fifo;
  // fifo = (queue *)q;
  Timer *T = (Timer *) q;
  fifo = T->q;

  srand(time(NULL));

  // double totalDrift = 0;
  struct timeval  timeValue, prodJobStart, prodJobEnd;
  double previousInsert, nextInsert;

  sleep(T->startDelay);
  gettimeofday(&timeValue, NULL);
  previousInsert = 1e6*timeValue.tv_sec + timeValue.tv_usec;
  nextInsert = previousInsert;

  // Every producer's thread run this loop, for LOOPS times, and every time add an object to queue buf
  for (int i = 0; i < T->tasksToExecute; i++) {
    pthread_mutex_lock (fifo->mut); // Until the producer's thread add the object, the queue is block
    gettimeofday(&prodJobStart, NULL);
    while (fifo->full) {
      printf ("producer: FULL queue.\n");
      // usleep(1000000);
      T->errorFnc(&jobsLostCounter);
      pthread_cond_wait (fifo->notFull, fifo->mut); // if the queue is full, the thread wait here, until a consumer's thread delete an object
    }

    gettimeofday(&timeValue, NULL);
    previousInsert = 1e6*timeValue.tv_sec + timeValue.tv_usec;

    workFunction wf;
    // Choose a random function from the array functions[NUM_OF_FUNCTIONS]
    // int rand_num = (int)(rand()%NUM_OF_FUNCTIONS);
    // wf.work = functions[rand_num];
    wf.work = T->timerFnc;

    // Choose a random argument for the fuction
    int rand_num = (int)(rand()%NUM_OF_ARGS);
    wf.arg = &arguments[rand_num];

    //Getting the arrival time at the queue
    gettimeofday(&(arrTime[(fifo->tail)]), NULL);
    gettimeofday(&prodJobEnd, NULL);

    queueAdd(fifo, wf);

    //Calculate the time taken for a producer to push a job to the queue
    int prodJobADD = (prodJobEnd.tv_sec-prodJobStart.tv_sec)*(int)1e6 + prodJobEnd.tv_usec-prodJobStart.tv_usec;
    metrixArrayAdd(f4, prodJobADD);
    printf("\nProducer push waiting time : %d  \n " , prodJobADD);

    pthread_mutex_unlock(fifo->mut);
    pthread_cond_signal(fifo->notEmpty);  // When the producer's thread add an object, send this signal to unlock a thread in consumer()

    double driftTime = previousInsert - nextInsert;
    metrixArrayAdd(f2,driftTime);
    printf("Drift time : %d \n " , (int)driftTime);

    double sleepTime = T->period - driftTime*1e-3;
    if(sleepTime > 0){
      usleep(sleepTime*(int)1e3);
      //printf("Drift time : %lf \n" , sleepTime);
    }
    else{
      continue;
      //printf("NO SLEEP \n");
    }
    //Update time value
    gettimeofday(&timeValue, NULL);
    nextInsert = 1e6*timeValue.tv_sec + timeValue.tv_usec;
  }

  counter++;
  // Check if all producer's threads have finish

  if(counter == P) {
    flag = 1;      // the final thread change the flag
    pthread_cond_broadcast(fifo->notEmpty);
  }

  return (NULL);
}


// implementation of producer
/*
 * parameters:
 * *q -> pointer to queue
*/
void *consumer (void *q)
{
  queue *fifo;
  // fifo = (queue *)q;

  consumerArg *conArg;
  conArg = (consumerArg *)q;
  fifo = conArg->q;

  int waitingTime ;
  struct timeval JobExecStart, JobExecEnd;

  // Use while 1 in order to take all the objects of queue's buffer
  while (1){
    pthread_mutex_lock (fifo->mut);

    while(fifo->empty==1 && flag != 1) {
      // printf ("consumer: queue EMPTY.\n");
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }

    // if flag == 1 -> all producer's threads have finish
    // and if fifo is empty
    // every thread unlock the mutex and after return
    if(flag == 1 && fifo->empty==1) {
      pthread_mutex_unlock (fifo->mut);
      pthread_cond_broadcast(fifo->notEmpty); // this signal, unlock all the threads in cond_wait
      break;
    }

    workFunction wf;

    struct timeval leaveTime;
    //Getting the leave time from the queueegettimeofday(&JobExecStart,NULL);
    gettimeofday(&leaveTime,NULL);
    //Calculating the waiting time at the queue
    waitingTime= (leaveTime.tv_sec -(arrTime[fifo->head]).tv_sec) *1e6 + (leaveTime.tv_usec-(arrTime[fifo->head]).tv_usec) ;
    printf("The waiting time is : %d  \n " , waitingTime);
    metrixArrayAdd(f1,waitingTime);

    queueDel (fifo, &wf);

    tempTime += waitingTime;

    // remaining_time_counter++;
    // printf("\n%d. remaining_time: %ld\n", remaining_time_counter, wf.remaining_time); // Using clock() funtion to take remaining time
    // printf("%lf\n", wf.remaining_time);

    // printf("\n%d. remaining_time: %ld\n", remaining_time_counter, wf.remaining_time); // Using gettimeofday() to take remaining time
    // printf("%ld\n", wf.remaining_time);

    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);

    gettimeofday(&JobExecStart,NULL);
    wf.work(wf.arg);
    gettimeofday(&JobExecEnd,NULL);

    pthread_mutex_lock(conArg->consumerMut);
    int JobDur = (JobExecEnd.tv_sec-JobExecStart.tv_sec)*(int)1e6 + JobExecEnd.tv_usec-JobExecStart.tv_usec;
    printf("Execution time is  : %d  \n " , JobDur);
    metrixArrayAdd(f3,JobDur);
    pthread_mutex_unlock(conArg->consumerMut);

  }

  printf("Consumer: RETURNNNNN !!!!!! \n\n");
  return (NULL);
}



int main (int argc, char* argv[])
{
  queue *fifo;
  pthread_t pro[P], con[Q];


  functions[0] = sum;       functions[5] = cosine;
  functions[1] = subtract;  functions[6] = printfunc;
  functions[2] = mul;       functions[7] = factorial;
  functions[3] = division;  functions[8] = square_root;
  functions[4] = sine;      functions[9] = even_odd_number;



  //Available timer's period in mseconds
  int period[3] = {1000, 100, 10};
  int mode = MODE;

  if (mode!=1 && mode!=2 && mode!=3 && mode!=4) {
      printf(" ERROR: Try again \n");// // Work function implementations
      exit(0);
  }

  int jobsToExecute = 0;
  switch (mode)
  {
    case 1:
      jobsToExecute = RUN_TIME * (int)1e3 / period[0];
      break;
    case 2:
      jobsToExecute = RUN_TIME * (int)1e3 / period[1];
      break;
    case 3:
      jobsToExecute = RUN_TIME * (int)1e3 / period[2];
      break;
    case 4:
      jobsToExecute = RUN_TIME * (int)1e3 / period[0] +  RUN_TIME * (int)1e3 / period[1] +  RUN_TIME * (int)1e3 / period[2];
      break;
  }

  f1 = fopen("TimeInQueue.txt", "w");
  f2 = fopen("DriftTime.txt", "w");
  f3 = fopen("JobExecTime.txt", "w");
  f4 = fopen("ProdWaitTime.txt", "w");

  fifo = queueInit();
  if(fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit(1);
  }

  // printf("remaining_time,\n");

  consumerArg *conArg = (consumerArg *) malloc( Q * sizeof(consumerArg));

  // Create Q consumer's threads first
  for(int i=0; i<Q; i++) {
    conArg[i].q = fifo;
    conArg[i].consumerMut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    // elegxos gia memory leak
    pthread_mutex_init(conArg[i].consumerMut, NULL);
    pthread_create( &con[i], NULL, consumer, (void *)(conArg + i));
  }

  Timer *T = (Timer*) malloc(sizeof(Timer));
  int function_id = rand() % NUM_OF_FUNCTIONS;

  conArg = (consumerArg *) malloc( Q * sizeof(consumerArg));

  switch(mode) {
    case 1:
      T = (Timer *)malloc(sizeof(Timer));
      *T = *timerInit(period[0], jobsToExecute , 0, fifo , producer, error, functions[function_id]);
      start(T);
      break;
    case 2:
      T = (Timer *)malloc(sizeof(Timer));
      *T = *timerInit(period[1], jobsToExecute , 0, fifo , producer, error, functions[function_id]);
      start(T);
      break;
    case 3:
      T = (Timer *)malloc(sizeof(Timer));
      *T = *timerInit(period[2], jobsToExecute , 0, fifo , producer, error, functions[function_id]);
      start(T);
      break;
    case 4:
      T = (Timer *)malloc(3 * sizeof(Timer));

      // elegxos gia memory leak
      T[0] = *timerInit(period[0], RUN_TIME * (int)1e3 / period[0], 0, fifo, producer, error, functions[function_id]);
      T[1] = *timerInit(period[1], RUN_TIME * (int)1e3 / period[1], 0, fifo, producer, error, functions[function_id]);
      T[2] = *timerInit(period[2], RUN_TIME * (int)1e3 / period[2], 0, fifo, producer, error, functions[function_id]);
      start((T+0));
      start((T+1));
      start((T+2));

      break;
  }



  // Create P producer's threads
  // for(int i=0; i<P; i++) {
  //   pthread_create(&pro[i], NULL, producer, fifo);
  // }


  // join all threads to return
  for(int i=0; i<P; i++) {
    pthread_join(pro[i], NULL);
  }

  // join all threads to return
  for(int i=0; i<Q; i++) {
    pthread_join(con[i], NULL);
  }


  fclose(f1);
  fclose(f2);
  fclose(f3);
  fclose(f4);

  queueDelete (fifo);

  return 0;
}




metrixArray * metrixArrayInit(int n){
  metrixArray *metr = (metrixArray *) malloc( sizeof(metrixArray) );
  if( metr==NULL ) {
    printf("Error at memory allocation \n");
  }
  metr->index = 0;
  metr->buffer = (int *)malloc(sizeof(int)*n);
  if(metr->buffer == NULL){
    printf("Error at memory allocation \n");
  }

  return metr;
}

void metrixArrayAdd(FILE *file_to_write , int value){
  fprintf(file_to_write, "%d\n", value );
}
