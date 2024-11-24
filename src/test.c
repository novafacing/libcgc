#include <assert.h>
#include <libcgc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Test mathematics functionality
void test_math() {
  printf("Testing math functions...\n");
  double result = sin(3.14159265359 / 2);
  assert(result > 0.99 && result < 1.01);  // Should be approximately 1
  printf("Math test passed!\n\n");
}

// Test transmit/receive functionality
void test_transmit_receive() {
  printf("Testing transmit/receive...\n");

  int pipefd[2];
  assert(pipe(pipefd) == 0);

  const char *test_str = "test message";
  size_t tx_bytes = 0;
  size_t rx_bytes = 0;
  char buffer[100] = {0};

  int tx_ret = transmit(pipefd[1], test_str, strlen(test_str), &tx_bytes);
  assert(tx_ret == 0 && tx_bytes == strlen(test_str));

  int rx_ret = receive(pipefd[0], buffer, sizeof(buffer), &rx_bytes);
  assert(rx_ret == 0 && rx_bytes == strlen(test_str));
  assert(memcmp(buffer, test_str, rx_bytes) == 0);

  close(pipefd[0]);
  close(pipefd[1]);

  printf("Transmit/receive test passed!\n\n");
}

// Test fd_set and fdwait functionality
void test_fdwait() {
  printf("Testing fdwait...\n");

  int pipefd[2];
  assert(pipe(pipefd) == 0);

  fd_set readfds;
  fd_set writefds;
  struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
  int ready = 0;

  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  FD_SET(pipefd[0], &readfds);   // read end
  FD_SET(pipefd[1], &writefds);  // write end

  int wait_ret = fdwait(pipefd[1] + 1, &readfds, &writefds, &timeout, &ready);
  assert(wait_ret == 0);
  assert(ready > 0);  // Write end should be ready
  assert(FD_ISSET(pipefd[1], &writefds));

  close(pipefd[0]);
  close(pipefd[1]);

  printf("FDwait test passed!\n\n");
}

// Test memory allocation/deallocation
void test_memory() {
  printf("Testing memory allocation...\n");
  void *addr = NULL;
  const size_t alloc_size = 4096;

  int alloc_ret = allocate(alloc_size, 0, &addr);
  assert(alloc_ret == 0 && addr != NULL);

  // Write to memory to ensure it's accessible
  memset(addr, 0x41, alloc_size);

  int dealloc_ret = deallocate(addr, alloc_size);
  assert(dealloc_ret == 0);

  printf("Memory allocation test passed!\n\n");
}

// Test random number generation
void test_random() {
  printf("Testing random number generation...\n");
  unsigned char rand_buf[16] = {0};
  size_t rand_bytes = 0;

  int rand_ret = random(rand_buf, sizeof(rand_buf), &rand_bytes);
  assert(rand_ret == 0 && rand_bytes == sizeof(rand_buf));

  // Verify we got some non-zero bytes (technically could fail, but probability
  // is negligible)
  int has_nonzero = 0;
  for (size_t i = 0; i < sizeof(rand_buf); i++) {
    if (rand_buf[i] != 0) {
      has_nonzero = 1;
      break;
    }
  }
  assert(has_nonzero);

  printf("Random number generation test passed!\n\n");
}

void test_flagpage() {
  printf("Testing flag page...\n");
  uint8_t *flag_page = (uint8_t *)FLAG_PAGE;
  size_t zeroes = 0;
  for (size_t i = 0; i < PAGE_SIZE; i++) {
    if (flag_page[i] == 0) {
      zeroes++;
    }
  }
  assert(zeroes < (PAGE_SIZE / 8));
  printf("Flag page test passed!\n\n");
}

int main() {
  printf("Starting LibCGC tests...\n\n");

  test_math();
  test_transmit_receive();
  test_fdwait();
  test_memory();
  test_random();
  test_flagpage();

  printf("All tests passed successfully!\n");
  return 0;
}