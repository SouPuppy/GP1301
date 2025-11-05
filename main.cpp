#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

/* debug info (ansi prefix for different threads) */
const char *DEBUG_CAMERA = "\033[1;32m▪\033[0m [C]";
const char *DEBUG_ESTIMATOR = "\033[1;34m▪\033[0m [E]";
const char *DEBUG_TRANSFORMER = "\033[1;35m▪\033[0m [T]";

/* external function declarations */
extern "C" {

double* generate_frame_vector(int length);
double* compression(double* frame, int length);

}

/* global settings */
#define FRAME_WIDTH 3
#define FRAME_HEIGHT 4
#define FRAME_SIZE (FRAME_WIDTH * FRAME_HEIGHT)
#define FIFO_SIZE (FRAME_SIZE * 5)
#define CACHE_ENTRIES 5

/* global variables */

int temp_frame_buffer_frame_id = -1; // [0 - CACHE_ENTRIES-1], -1 = empty
double temp_frame_buffer[FRAME_SIZE];

volatile int job_done = 0; // 0 = in progress, 1 = done

pthread_mutex_t temp_frame_buffer_mutex;

sem_t cache_entries_empty_sem;
sem_t cache_entries_arrived_sem;

sem_t temp_frame_buffer_empty_sem;
sem_t temp_frame_buffer_ready_sem;

/* Resources */
struct FrameCache {
  enum fifoStatus { EMPTY = 0, ARRIVED = 1, DELETE = 2 };
  // entries 
  struct FIFO {
    double buffer[FRAME_SIZE];
    pthread_mutex_t lock;
  } entries[CACHE_ENTRIES];

  // the resource should be managed as FIFO queues
  int available_entries[CACHE_ENTRIES], available_head, available_rear;  // entries that are empty
  int arrived_entry[CACHE_ENTRIES]    , arrived_head  , arrived_rear;            // entries that are fully arrived in cache

  void acquire_entry(int entry_id) { pthread_mutex_lock(&entries[entry_id].lock); }
  void release_entry(int entry_id) { pthread_mutex_unlock(&entries[entry_id].lock); }

  FrameCache() {
    // initialize available entries
    available_head = 0;
    available_rear = CACHE_ENTRIES - 1;
    for (int i = 0; i < CACHE_ENTRIES; i++) {
      available_entries[i] = i;
      pthread_mutex_init(&entries[i].lock, NULL);
    }
    // initialize arrived entries
    arrived_head = 0;
    arrived_rear = 0;
  }
  
  void load_buffer(int entry_id, double *buffer, size_t len) {
    memcpy(entries[entry_id].buffer, buffer, len * sizeof(double));
    
    // push entry to arrived list
    arrived_entry[arrived_rear] = entry_id;
    arrived_rear = (arrived_rear + 1) % CACHE_ENTRIES;
  }

  int get_free_entry() {
    sem_wait(&cache_entries_empty_sem);
    int entry_id = available_entries[available_head];
    available_head = (available_head + 1) % CACHE_ENTRIES;
    return entry_id;
  }

  int get_arrived_entry() {
    sem_wait(&cache_entries_arrived_sem);
    int entry_id = arrived_entry[arrived_head];
    arrived_head = (arrived_head + 1) % CACHE_ENTRIES;
    return entry_id;
  }

  void flush(int entry_id) {
    available_rear = (available_rear + 1) % CACHE_ENTRIES;
    available_entries[available_rear] = entry_id;
    sem_post(&cache_entries_empty_sem);
  }

} gFrameCache;

/* Threads */
void *T_Camera(void *arg) { 
  int interval = *((int *)arg);

  double *frame;
  while (!job_done) {
    printf("%s Capturing frame...\n", DEBUG_CAMERA);
    frame = generate_frame_vector(FRAME_SIZE);
    if (frame == nullptr) { 
      job_done = 1;
      sem_post(&cache_entries_arrived_sem);
      break;
    }
    // get a valid cache entry
    int entry_id = gFrameCache.get_free_entry();

    // write data into cache entries
    gFrameCache.acquire_entry(entry_id);
    
    // transfer camera buffer to cache
    gFrameCache.load_buffer(entry_id, frame, FRAME_SIZE);

    gFrameCache.release_entry(entry_id);

    // signal one filled entry (for transformer later)
    sem_post(&cache_entries_arrived_sem);

    // sleep for interval
    sleep(interval);
  }
  return nullptr;
}

void *T_Estimator(void *arg) { 
  while (!job_done) {
    sem_wait(&temp_frame_buffer_ready_sem);
    if (job_done) {
      sem_post(&temp_frame_buffer_empty_sem);
      break;
    }

    pthread_mutex_lock(&temp_frame_buffer_mutex);
    printf("%s Estimator got framebuffer with frame id %d\n", DEBUG_ESTIMATOR, temp_frame_buffer_frame_id);
    
gFrameCache.acquire_entry(temp_frame_buffer_frame_id);

    // estimate the cache and temp framebuffer with MSE
    double mse = 0.0;
    for (int i = 0; i < FRAME_SIZE; i++) {
      mse += (temp_frame_buffer[i] - gFrameCache.entries[temp_frame_buffer_frame_id].buffer[i]) *
             (temp_frame_buffer[i] - gFrameCache.entries[temp_frame_buffer_frame_id].buffer[i]);
    }
    mse /= FRAME_SIZE;
    printf("mse = %.6f\n", mse);

gFrameCache.release_entry(temp_frame_buffer_frame_id);
    
    // flush the entry
    gFrameCache.flush(temp_frame_buffer_frame_id);

    sem_post(&temp_frame_buffer_empty_sem);
    pthread_mutex_unlock(&temp_frame_buffer_mutex);
  }
  return nullptr;
}

void *T_Transformer(void *arg) { 
  while (!job_done) {
    // get framebuffer lock (for write protection)
    sem_wait(&temp_frame_buffer_empty_sem);
    if (job_done) {
      break;
    }
    pthread_mutex_lock(&temp_frame_buffer_mutex);

    // fetch from cache
    int entry_id = gFrameCache.get_arrived_entry();

gFrameCache.acquire_entry(entry_id);

    printf("%s Transformer got cache entry %d\n", DEBUG_TRANSFORMER, entry_id);

    // load data from cache to framebuffer
    temp_frame_buffer_frame_id = entry_id;
    memcpy(temp_frame_buffer, gFrameCache.entries[entry_id].buffer, FRAME_SIZE * sizeof(double));
    
gFrameCache.release_entry(entry_id);

    // pseudo compression and decompression
    compression(temp_frame_buffer, FRAME_SIZE);
    
    // notice estimator that frame is ready
    pthread_mutex_unlock(&temp_frame_buffer_mutex);
    sem_post(&temp_frame_buffer_ready_sem);
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
  pthread_mutex_init(&temp_frame_buffer_mutex, NULL);

  sem_init(&cache_entries_arrived_sem, 0, 0);
  sem_init(&cache_entries_empty_sem, 0, CACHE_ENTRIES);
  
  sem_init(&temp_frame_buffer_empty_sem, 0, 1);
  sem_init(&temp_frame_buffer_ready_sem, 0, 0);

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
