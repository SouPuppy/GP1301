#include <cstdio>
#include <cstdlib>

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
struct T_Camera {};
struct T_Estimator {};
struct T_Transformer {};

/* main */
int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <interval>\n", argv[0]);
    return -1;
  }
  int interval = atoi(argv[1]);
  printf("Interval: %d sec\n", interval);
  return 0;
}
