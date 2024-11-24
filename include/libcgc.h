#ifndef _LIBCGC_H
#define _LIBCGC_H

#ifndef _NO_STD_DEFS
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED

#if defined(__LP64__) || defined(_LP64)
typedef unsigned long size_t;
#define STD_SIZE_T unsigned long
#else
typedef unsigned int size_t;
#define STD_SIZE_T unsigned int
#endif

#define SIZE_MAX __SIZE_MAX__
#define SSIZE_MAX (__SIZE_MAX__ >> 1)

#endif /* _SIZE_T_DEFINED */

#ifndef _FD_SET_DEFINED
#define _FD_SET_DEFINED

/* Number of fds in an fd_set */
#define FD_SETSIZE 1024

#ifndef fd_set
typedef struct {
  unsigned long fds_bits[FD_SETSIZE / (8 * sizeof(unsigned long))];
} fd_set;
#endif

/* Macro implementations for fd_set operations */
#define FD_ZERO(set)                                                       \
  do {                                                                     \
    size_t __i;                                                            \
    for (__i = 0; __i < (FD_SETSIZE / (8 * sizeof(unsigned long))); __i++) \
      ((set)->fds_bits)[__i] = 0;                                          \
  } while (0)

#define FD_SET(fd, set)                                          \
  ((void)((set)->fds_bits[(fd) / (8 * sizeof(unsigned long))] |= \
          (1UL << ((fd) % (8 * sizeof(unsigned long))))))

#define FD_CLR(fd, set)                                          \
  ((void)((set)->fds_bits[(fd) / (8 * sizeof(unsigned long))] &= \
          ~(1UL << ((fd) % (8 * sizeof(unsigned long))))))

#define FD_ISSET(fd, set)                                \
  ((set)->fds_bits[(fd) / (8 * sizeof(unsigned long))] & \
   (1UL << ((fd) % (8 * sizeof(unsigned long)))))

#endif /* _FD_SET_DEFINED */

#ifndef _TIMEVAL_DEFINED
#define _TIMEVAL_DEFINED

struct timeval {
  long tv_sec;  /* seconds */
  long tv_usec; /* microseconds */
};

#endif /* _TIMEVAL_DEFINED */

#ifndef _JMP_BUF_DEFINED
#define _JMP_BUF_DEFINED

#if defined(__i386__)
typedef int __jmp_buf[6];
#elif defined(__x86_64__)
typedef long __jmp_buf[8];
#elif defined(__arm__)
typedef int __jmp_buf[64];
#elif defined(__aarch64__)
typedef long __jmp_buf[32];
#else
#error "Architecture not supported"
#endif

typedef struct __jmp_buf_tag {
  __jmp_buf __jb;
  unsigned long __fl;
  unsigned long __ss[128 / sizeof(long)];
} jmp_buf[1];

#endif /* _JMP_BUF_DEFINED */
#endif /* _NO_STD_DEFS */

#define FLAG_PAGE (0x4347C000)
#define PAGE_SIZE (0x1000)

extern int setjmp(jmp_buf) __attribute__((__returns_twice__));
extern void longjmp(jmp_buf, int) __attribute__((__noreturn__));

extern float sinf(float);
extern double sin(double);
extern long double sinl(long double);
extern float cosf(float);
extern double cos(double);
extern long double cosl(long double);
extern float tanf(float);
extern double tan(double);
extern long double tanl(long double);
extern float logf(float);
extern double log(double);
extern long double logl(long double);
extern float rintf(float);
extern double rint(double);
extern long double rintl(long double);
extern float sqrtf(float);
extern double sqrt(double);
extern long double sqrtl(long double);
extern float fabsf(float);
extern double fabs(double);
extern long double fabsl(long double);
extern float log2f(float);
extern double log2(double);
extern long double log2l(long double);
extern float exp2f(float);
extern double exp2(double);
extern long double exp2l(long double);
extern float expf(float);
extern double exp(double);
extern long double expl(long double);
extern float log10f(float);
extern double log10(double);
extern long double log10l(long double);
extern float powf(float, float);
extern double pow(double, double);
extern long double powl(long double, long double);
extern float atan2f(float, float);
extern double atan2(double, double);
extern long double atan2l(long double, long double);
extern float remainderf(float, float);
extern double remainder(double, double);
extern long double remainderl(long double, long double);
extern float scalbnf(float, int);
extern double scalbn(double, int);
extern long double scalbnl(long double, int);
extern float scalblnf(float, long int);
extern double scalbln(double, long int);
extern long double scalblnl(long double, long int);
extern float significandf(float);
extern double significand(double);
extern long double significandl(long double);

void _terminate(unsigned int status) __attribute__((__noreturn__));
int transmit(int fd, const void *buf, size_t count, size_t *tx_bytes);
int receive(int fd, void *buf, size_t count, size_t *rx_bytes);
int fdwait(int nfds, fd_set *readfds, fd_set *writefds,
           const struct timeval *timeout, int *readyfds);
int allocate(size_t length, int is_X, void **addr);
int deallocate(void *addr, size_t length);
int random(void *buf, size_t count, size_t *rnd_bytes);

#endif /* _LIBCGC_H */
