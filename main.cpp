#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

/* external function declarations */
extern "C" {

double* generate_frame_vector(int length);
double* compression(double* frame, int length);

}

/* global settings */
#define FRAME_WIDTH 800
#define FRAME_HEIGHT 600
#define FRAME_SIZE (FRAME_WIDTH * FRAME_HEIGHT)
#define FIFO_SIZE (FRAME_SIZE * 5)
#define CACHE_ENTRIES 5

/* locks */
pthread_mutex_t frame_buffer_mutex;

/* FIFO */
struct FIFO {
  double *buffer;
  FIFO() { buffer = new double[FIFO_SIZE]; }
  void push(double _) { /* TODO */ }
  double pop() { return 0.0; /* TODO */ }
  ~FIFO() { delete[] buffer; }
};

/* Resources */
struct Cache {
  // entries
  struct {
    FIFO buffer;
    int status; // 0 - empty, 1 - filled, 2 - completed
    pthread_mutex_t lock;
  } entries[CACHE_ENTRIES];

  int available_entries[CACHE_ENTRIES], available_count;  // entries that are empty
  int complete_entries[CACHE_ENTRIES], complete_count;    // entries that are completed

  Cache() {
    available_count = CACHE_ENTRIES;
    for (int i = 0; i < CACHE_ENTRIES; i++) available_entries[i] = i;
  }
  int get_entry() { if (available_count <= 0) return -1; return available_entries[--available_count]; }
  int return_entry(int idx) { available_entries[available_count++] = idx;  return 0; }
};

double frame_buffer[FRAME_SIZE];

/* Threads */
void *T_Camera(void *arg) { 
  double *frame = generate_frame_vector(FRAME_SIZE);
  printf("[ info ] generated frame at %p\n", frame);
  return nullptr;
}

void *T_Estimator(void *arg) { printf("[ info ] Estimator thread finished\n"); return nullptr; }
void *T_Transformer(void *arg) { printf("[ info ] Transformer thread finished\n"); return nullptr; }

pthread_t camera, estimator, transformer;

/* main */
int main(int argc, char *argv[]) {
  /* parse arguments */
  if (argc != 2) {
    printf("Usage: %s <interval>\n", argv[0]);
    return -1;
  }
  int interval = atoi(argv[1]);

  /* init locks */
  pthread_mutex_init(&frame_buffer_mutex, NULL);

  /* init threads */
  int rc;
  rc = pthread_create(&camera, NULL, T_Camera, (void *)&interval);  if (rc) { printf("Error: Unable to create camera thread, %d\n", rc); exit(-1); }
  rc = pthread_create(&estimator, NULL, T_Estimator, NULL);         if (rc) { printf("Error: Unable to create estimator thread, %d\n", rc); exit(-1); }
  rc = pthread_create(&transformer, NULL, T_Transformer, NULL);     if (rc) { printf("Error: Unable to create transformer thread, %d\n", rc); exit(-1); }

  /* wait for threads to finish */
  rc = pthread_join(camera, NULL);                                  if (rc) { printf("Error: Unable to join camera thread, %d\n", rc); exit(-1); }
  rc = pthread_join(estimator, NULL);                               if (rc) { printf("Error: Unable to join estimator thread, %d\n", rc); exit(-1); }
  rc = pthread_join(transformer, NULL);                             if (rc) { printf("Error: Unable to join transformer thread, %d\n", rc); exit(-1); }

  return 0;
}
