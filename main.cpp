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
sem_t cache_entries_arrived_sem;

/* Resources */
struct FrameCache {
  enum fifoStatus { EMPTY = 0, ARRIVED = 1, DELETE = 2 };
  // entries 
  // FIXME - this is not complete yet
  struct FIFO {
    double buffer[FRAME_SIZE];
    fifoStatus status;
    pthread_mutex_t lock;
  } entries[CACHE_ENTRIES];

  int available_entries[CACHE_ENTRIES], available_count;  // entries that are empty
  int complete_entries[CACHE_ENTRIES], arrived_count;     // entries that are completed

  void lock_entry(int entry_id) { pthread_mutex_lock(&entries[entry_id].lock); }
  void unlock_entry(int entry_id) { pthread_mutex_unlock(&entries[entry_id].lock); }
  void set_stat(int entry_id, fifoStatus stat) { entries[entry_id].status = stat; }

  FrameCache() {
    available_count = CACHE_ENTRIES;
    arrived_count = 0;
    for (int i = 0; i < CACHE_ENTRIES; i++) {
      available_entries[i] = i;
      pthread_mutex_init(&entries[i].lock, NULL);
      entries[i].status = EMPTY;
    }
  }
  
  void load_buffer(int entry_id, double *buffer, size_t len) {
    memcpy(entries[entry_id].buffer, buffer, len * sizeof(double));
    entries[entry_id].status = ARRIVED;
  }

  int get_free_entry() {
    printf("[sem] waiting...\n");
    sem_wait(&cache_entries_empty_sem);
    printf("[sem] acquired!\n");
    return available_entries[--available_count];
  }

  int get_arrived_entry() {
    sem_wait(&cache_entries_arrived_sem);
    return complete_entries[--arrived_count];
  }
} gFrameCache;

double frame_buffer[FRAME_SIZE];

/* Threads */
void *T_Camera(void *arg) { 
  int interval = *((int *)arg);

  double *frame;
  while (!job_done) {
    frame = generate_frame_vector(FRAME_SIZE);
    if (frame == nullptr) { job_done = 1; break; }
    
    // get a valid cache entry
    int entry_id = gFrameCache.get_free_entry();

    printf("[ info ] get cache entry %d\n", entry_id);
    // write data into cache entries
    gFrameCache.lock_entry(entry_id);
    
    // transfer camera buffer to cache
    gFrameCache.load_buffer(entry_id, frame, FRAME_SIZE);

    gFrameCache.set_stat(entry_id, FrameCache::ARRIVED);
    gFrameCache.unlock_entry(entry_id);

    // signal one filled entry (for transformer later)
    sem_post(&cache_entries_arrived_sem);

    // sleep for interval
    printf("[ info ] sleeping for %d s...\n", interval);
    sleep(interval);
  }
  return nullptr;
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

  sem_init(&cache_entries_arrived_sem, 0, 0);
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
