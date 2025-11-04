#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

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

/* global variables */
volatile int job_done = 0; // 0 = in progress, 1 = done

pthread_mutex_t frame_buffer_mutex;

sem_t cache_entries_empty_sem;
sem_t cache_entries_full_sem;


/* FIFO */
struct FIFO {
  double *buffer;
  size_t head, rear, count;
  FIFO() { buffer = new double[FIFO_SIZE]; }
  void enqueue_bu(double *buff, size_t len) { /* TODO */ }
  double dequeue(double *buff, size_t len) { return 0.0; /* TODO */ }
  ~FIFO() { delete[] buffer; }
};

/* Resources */
struct FrameCache {
  enum fifoStatus { EMPTY = 0, ARRIVED = 1, DELETE = 2 };
  // entries
  struct {
    FIFO buffer;
    fifoStatus status;
    pthread_mutex_t lock;
  } entries[CACHE_ENTRIES];

  int available_entries[CACHE_ENTRIES], available_count;  // entries that are empty
  int complete_entries[CACHE_ENTRIES], complete_count;    // entries that are completed

  FrameCache() {
    available_count = CACHE_ENTRIES + 1;
    for (int i = 0; i < CACHE_ENTRIES; i++) available_entries[i] = i;
  }
  void load_buffer(int entry_id, double *buffer, size_t len) { memcpy(entries[entry_id].buffer.buffer, buffer, len * sizeof(double)); }
  void lock_entry(int entry_id) { pthread_mutex_lock(&entries[entry_id].lock); }
  void unlock_entry(int entry_id) { pthread_mutex_unlock(&entries[entry_id].lock); }
  void set_stat(int entry_id, fifoStatus stat) { entries[entry_id].status = stat; }
  int get_free_entry() { /* get empty entry */ return 4; }
} gFrameCache;

double frame_buffer[FRAME_SIZE];

/* Threads */
void *T_Camera(void *arg) { 
  int interval = *((int *)arg);

  double *frame;
  while (!job_done) {
    frame = generate_frame_vector(FRAME_SIZE);
    if (frame == nullptr) { job_done = 1; break; }
    
    // wait for a valid cache entries
    int entry_id = gFrameCache.get_free_entry();

    printf("[ info ] get cache entry %d\n", entry_id);
    // write data into cache entries
    gFrameCache.lock_entry(entry_id);
    
    // transfer camera buffer to cache
    gFrameCache.load_buffer(entry_id, frame, FRAME_SIZE);

    gFrameCache.set_stat(entry_id, FrameCache::ARRIVED);
    gFrameCache.unlock_entry(entry_id);
    // sleep for interval
    printf("[ info ] sleeping for %d s...\n", interval);
    sleep(interval);
  }
}

void *T_Estimator(void *arg) { 
  while (!job_done) {
    // fetch from cache [stat = arrived]
    // wait for frame buffer to be empty
    // cache -> compression -> decompress -> framebuffer
    // cache [stat => activate], frambuffer_id => entries_id
  }
  return nullptr;
}

void *T_Transformer(void *arg) { 
  while (!job_done) {
    // fetch frambuffer
    // get framebuffer id
    // fetch origin buffer from cache
    // calc MSE and output
    // flush framebuffer
    // flush cache
  }
  return nullptr;
}

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

  sem_init(&cache_entries_full_sem, 0, 0);
  sem_init(&cache_entries_empty_sem, 0, CACHE_ENTRIES);


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
