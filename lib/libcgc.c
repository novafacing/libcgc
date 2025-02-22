#include <errno.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#define _NO_STD_DEFS
#include "libcgc.h"

extern void exit(int status) __attribute__((noreturn));

void _terminate(unsigned int status) { exit(status); }

int transmit(int fd, const void *buf, size_t count, size_t *tx_bytes) {
  const ssize_t ret = write(fd, buf, count);

  if (ret < 0) {
    return errno;
  } else if (tx_bytes != NULL) {
    *tx_bytes = ret;
  }

  return 0;
}

int receive(int fd, void *buf, size_t count, size_t *rx_bytes) {
  ssize_t ret = read(fd, buf, count);

  if (ret < 0) {
    return errno;
  } else if (rx_bytes != NULL) {
    *rx_bytes = ret;
  }

  return 0;
}

static int check_timeout(const struct timeval *timeout) {
  if (!timeout) {
    return 0;
  } else if (0 > timeout->tv_sec || 0 > timeout->tv_usec) {
    return EINVAL;
  } else {
    return 0;
  }
}

#define _USEC_PER_SEC 1000000L
#define _NSEC_PER_USEC 1000L

int fdwait(int nfds, fd_set *readfds, fd_set *writefds,
           const struct timeval *timeout, int *readyfds) {
  struct timeval tv_work;
  if (timeout != NULL) {
    tv_work = *timeout;
    tv_work.tv_sec += tv_work.tv_usec / _USEC_PER_SEC;
    tv_work.tv_usec %= _USEC_PER_SEC;
    tv_work.tv_usec -= tv_work.tv_usec % (10 * _NSEC_PER_USEC);
    if (tv_work.tv_sec < 0 || tv_work.tv_usec < 0) {
      return EINVAL;
    }
  }

  int res = select(nfds, readfds, writefds, NULL, timeout ? &tv_work : NULL);

  if (res < 0) {
    return errno;
  }

  if (readyfds != NULL) {
    *readyfds = res;
  }

  return 0;
}

int allocate(size_t length, int is_X, void **addr) {
  void *res = mmap(NULL, length,
                   PROT_READ | PROT_WRITE | (is_X ? PROT_EXEC : PROT_NONE),
                   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (res == MAP_FAILED) {
    return errno;
  }

  if (res != NULL) {
    memset(res, 0, length);
    *addr = res;
  } else {
    return ENOMEM;
  }

  return 0;
}

int deallocate(void *addr, size_t length) {
  int ret = munmap(addr, length);

  if (ret < 0) {
    return errno;
  }

  return 0;
}

#define AES_BLOCK_SIZE 16
#define AES_ROUNDS 10
#define NK 4
#define NB 4

static const uint8_t SBOX[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b,
    0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26,
    0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2,
    0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed,
    0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f,
    0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec,
    0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14,
    0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d,
    0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f,
    0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11,
    0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f,
    0xb0, 0x54, 0xbb, 0x16};

static const uint8_t RCON[10] = {0x01, 0x02, 0x04, 0x08, 0x10,
                                 0x20, 0x40, 0x80, 0x1b, 0x36};

typedef struct {
  uint8_t key[AES_BLOCK_SIZE];
  uint32_t round_key[AES_ROUNDS + 1][4];
  uint8_t V[AES_BLOCK_SIZE];
  uint8_t DT[AES_BLOCK_SIZE];
  uint8_t prev_rand[AES_BLOCK_SIZE];
} PRNG_CTX;

static uint32_t rotword(uint32_t word) { return (word << 8) | (word >> 24); }

static uint32_t subword(uint32_t word) {
  return (SBOX[(word >> 24) & 0xFF] << 24) | (SBOX[(word >> 16) & 0xFF] << 16) |
         (SBOX[(word >> 8) & 0xFF] << 8) | SBOX[word & 0xFF];
}

static void aes_key_expansion(PRNG_CTX *ctx) {
  uint32_t temp;
  int i;

  for (i = 0; i < NK; i++) {
    ctx->round_key[0][i] = ((uint32_t)ctx->key[4 * i] << 24) |
                           ((uint32_t)ctx->key[4 * i + 1] << 16) |
                           ((uint32_t)ctx->key[4 * i + 2] << 8) |
                           ((uint32_t)ctx->key[4 * i + 3]);
  }

  for (i = NK; i < NB * (AES_ROUNDS + 1); i++) {
    temp = ctx->round_key[(i - 1) / NK][(i - 1) % NK];
    if (i % NK == 0) {
      temp = subword(rotword(temp)) ^ (RCON[(i / NK) - 1] << 24);
    }
    ctx->round_key[i / NK][i % NK] =
        ctx->round_key[(i - NK) / NK][i % NK] ^ temp;
  }
}

static void add_round_key(uint8_t state[4][4], uint32_t round_key[4]) {
  for (int i = 0; i < 4; i++) {
    uint32_t rk = round_key[i];
    state[0][i] ^= (rk >> 24) & 0xFF;
    state[1][i] ^= (rk >> 16) & 0xFF;
    state[2][i] ^= (rk >> 8) & 0xFF;
    state[3][i] ^= rk & 0xFF;
  }
}

static void sub_bytes(uint8_t state[4][4]) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      state[i][j] = SBOX[state[i][j]];
    }
  }
}

static void shift_rows(uint8_t state[4][4]) {
  uint8_t temp;

  temp = state[1][0];
  state[1][0] = state[1][1];
  state[1][1] = state[1][2];
  state[1][2] = state[1][3];
  state[1][3] = temp;

  temp = state[2][0];
  state[2][0] = state[2][2];
  state[2][2] = temp;
  temp = state[2][1];
  state[2][1] = state[2][3];
  state[2][3] = temp;

  temp = state[3][3];
  state[3][3] = state[3][2];
  state[3][2] = state[3][1];
  state[3][1] = state[3][0];
  state[3][0] = temp;
}

static uint8_t gmul(uint8_t a, uint8_t b) {
  uint8_t p = 0;
  for (int i = 0; i < 8; i++) {
    if (b & 1) {
      p ^= a;
    }
    uint8_t hi_bit_set = a & 0x80;
    a <<= 1;
    if (hi_bit_set) {
      a ^= 0x1B;
    }
    b >>= 1;
  }
  return p;
}

static void mix_columns(uint8_t state[4][4]) {
  uint8_t temp[4];
  for (int c = 0; c < 4; c++) {
    for (int i = 0; i < 4; i++) {
      temp[i] = state[i][c];
    }

    state[0][c] = gmul(0x02, temp[0]) ^ gmul(0x03, temp[1]) ^ temp[2] ^ temp[3];
    state[1][c] = temp[0] ^ gmul(0x02, temp[1]) ^ gmul(0x03, temp[2]) ^ temp[3];
    state[2][c] = temp[0] ^ temp[1] ^ gmul(0x02, temp[2]) ^ gmul(0x03, temp[3]);
    state[3][c] = gmul(0x03, temp[0]) ^ temp[1] ^ temp[2] ^ gmul(0x02, temp[3]);
  }
}

static void aes_encrypt_block(PRNG_CTX *ctx, uint8_t in[16], uint8_t out[16]) {
  uint8_t state[4][4];
  int i, j, round;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      state[j][i] = in[i * 4 + j];
    }
  }

  // Initial round
  add_round_key(state, ctx->round_key[0]);

  // Main rounds
  for (round = 1; round < AES_ROUNDS; round++) {
    sub_bytes(state);
    shift_rows(state);
    mix_columns(state);
    add_round_key(state, ctx->round_key[round]);
  }

  // Final round
  sub_bytes(state);
  shift_rows(state);
  add_round_key(state, ctx->round_key[AES_ROUNDS]);

  // Copy state to output
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      out[i * 4 + j] = state[j][i];
    }
  }
}

// Initialize PRNG context
static void prng_init(PRNG_CTX *ctx, const uint8_t *key, const uint8_t *seed) {
  memcpy(ctx->key, key ? key : (uint8_t *)"0123456789abcdef", AES_BLOCK_SIZE);
  memcpy(ctx->V, seed ? seed : (uint8_t *)"zaybxcwdveuftgsh", AES_BLOCK_SIZE);
  memset(ctx->DT, 0, AES_BLOCK_SIZE);
  memset(ctx->prev_rand, 0, AES_BLOCK_SIZE);

  // Initialize timestamp in DT
  uint64_t timestamp = time(NULL);
  memcpy(ctx->DT, &timestamp, sizeof(timestamp));

  aes_key_expansion(ctx);
  return;
}

// Generate random bytes
static int prng_generate(PRNG_CTX *ctx, uint8_t *output, size_t len) {
  uint8_t I[AES_BLOCK_SIZE], rand[AES_BLOCK_SIZE];
  size_t generated = 0;

  while (generated < len) {
    // Step 1: Encrypt timestamp
    aes_encrypt_block(ctx, ctx->DT, I);

    // Step 2: XOR I with V and encrypt
    for (int i = 0; i < AES_BLOCK_SIZE; i++) {
      rand[i] = I[i] ^ ctx->V[i];
    }
    aes_encrypt_block(ctx, rand, rand);

    // Check for repetition
    if (memcmp(rand, ctx->prev_rand, AES_BLOCK_SIZE) == 0) {
      return -1;
    }
    memcpy(ctx->prev_rand, rand, AES_BLOCK_SIZE);

    // Step 3: Update V
    for (int i = 0; i < AES_BLOCK_SIZE; i++) {
      ctx->V[i] = rand[i] ^ I[i];
    }
    aes_encrypt_block(ctx, ctx->V, ctx->V);

    // Copy output
    size_t copy_len = len - generated;
    if (copy_len > AES_BLOCK_SIZE) {
      copy_len = AES_BLOCK_SIZE;
    }
    memcpy(output + generated, rand, copy_len);
    generated += copy_len;

    // Increment DT
    for (int i = AES_BLOCK_SIZE - 1; i >= 0; i--) {
      ctx->DT[i]++;
      if (ctx->DT[i] != 0) {
        break;
      }
    }
  }
  return generated;
}

static PRNG_CTX prng_ctx = {0};
static int prng_initialized = 0;

int random(void *buf, size_t count, size_t *rnd_bytes) {
  if (!prng_initialized) {
    prng_init(&prng_ctx, NULL, NULL);
    prng_initialized = 1;
  }

  int res = prng_generate(&prng_ctx, (uint8_t *)buf, count);

  if (res < 0) {
    return -1;
  }

  *rnd_bytes = res;

  return 0;
}

static void __attribute__((constructor)) initialize_flag_page(void) {
  void *flag_page = mmap((void *)FLAG_PAGE, PAGE_SIZE, PROT_READ | PROT_WRITE,
                         MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (flag_page != (void *)FLAG_PAGE) {
    _terminate(1);
  }

  size_t rnd_bytes = 0;
  if (random(flag_page, PAGE_SIZE, &rnd_bytes) < 0) {
    _terminate(1);
  }

  return;
}

void _force_libm_link(void) {
  float f = 0.0;
  double d = 0.0;
  long double ld = 0.0;
  int i = 0;
  long int li = 0;
  sinf(f);
  sin(d);
  sinl(ld);
  cosf(f);
  cos(d);
  cosl(ld);
  tanf(f);
  tan(d);
  tanl(ld);
  logf(f);
  log(d);
  logl(ld);
  rintf(f);
  rint(d);
  rintl(ld);
  sqrtf(f);
  sqrt(d);
  sqrtl(ld);
  fabsf(f);
  fabs(d);
  fabsl(ld);
  log2f(f);
  log2(d);
  log2l(ld);
  exp2f(f);
  exp2(d);
  exp2l(ld);
  expf(f);
  exp(d);
  expl(ld);
  log10f(f);
  log10(d);
  log10l(ld);
  powf(f, f);
  pow(d, d);
  powl(ld, ld);
  atan2f(f, f);
  atan2(d, d);
  atan2l(ld, ld);
  remainderf(f, f);
  remainder(d, d);
  remainderl(ld, ld);
  scalbnf(f, i);
  scalbn(d, i);
  scalbnl(ld, i);
  scalblnf(f, li);
  scalbln(d, li);
  scalblnl(ld, li);
  significandf(f);
  significand(d);
  significandl(ld);
}
