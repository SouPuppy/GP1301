#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

/* global settings */
#define FRAME_WIDTH 800
#define FRAME_HEIGHT 600
#define FRAME_SIZE (FRAME_WIDTH * FRAME_HEIGHT)
#define FIFO_SIZE (FRAME_SIZE * 5)
#define CACHE_ENTRIES 5

/* FIFO */
struct FIFO {
  double *buffer;
  FIFO() { buffer = new double[FIFO_SIZE]; }
  void push(double _) { /* TODO */ }
  double pop() { return 0.0; /* TODO */ }
  ~FIFO() { delete[] buffer; }
};

/* Threads */
void *T_Camera(void *arg) { printf("[ info ] Camera thread finished\n"); return nullptr; }
void *T_Estimator(void *arg) { printf("[ info ] Estimator thread finished\n"); return nullptr; }
void *T_Transformer(void *arg) { printf("[ info ] Transformer thread finished\n"); return nullptr; }

pthread_t camera, estimator, transformer;

/* main */
int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <interval>\n", argv[0]);
    return -1;
  }

  int interval = atoi(argv[1]);

  /* init threads */
  printf("[ info ] initializing threads...\n");

  int rc;
  rc = pthread_create(&camera, NULL, T_Camera, (void *)&interval);  if (rc) { printf("Error: Unable to create camera thread, %d\n", rc); exit(-1); }
  rc = pthread_create(&estimator, NULL, T_Estimator, NULL);         if (rc) { printf("Error: Unable to create estimator thread, %d\n", rc); exit(-1); }
  rc = pthread_create(&transformer, NULL, T_Transformer, NULL);     if (rc) { printf("Error: Unable to create transformer thread, %d\n", rc); exit(-1); }

  printf("[ info ] threads initialized\n");


  /* wait for threads to finish */
  printf("[ info ] waiting for threads to finish...\n");
  rc = pthread_join(camera, NULL);                                  if (rc) { printf("Error: Unable to join camera thread, %d\n", rc); exit(-1); }
  rc = pthread_join(estimator, NULL);                               if (rc) { printf("Error: Unable to join estimator thread, %d\n", rc); exit(-1); }
  rc = pthread_join(transformer, NULL);                             if (rc) { printf("Error: Unable to join transformer thread, %d\n", rc); exit(-1); }

  printf("[ info ] all threads finished\n");

  return 0;
}
