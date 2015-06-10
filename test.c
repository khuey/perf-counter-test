#define _GNU_SOURCE 1

#include <assert.h>
#include <errno.h>
#include <linux/perf_event.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int start_counter() {
  struct perf_event_attr attr;
  memset(&attr, 0, sizeof(attr));
  attr.type = PERF_TYPE_RAW;
  attr.size = sizeof(attr);
  // Use the processor's raw branch counter
#if defined(__arm__)
  attr.config = 0x0D;
#else
  attr.config = 0x5101c4;
#endif
  // We require counting userspace code only.
  attr.exclude_kernel = 1;
  attr.exclude_guest = 1;

  int fd = syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
  if (fd == -1) {
    printf("errno: %d\n", errno);
  }
  assert(fd > 0);
  assert(!ioctl(fd, PERF_EVENT_IOC_ENABLE, 0));
  return fd;
}

void timing_computation() {
  for (int counter = 0; counter < 100000000; counter++) {
    if (counter % 100000 == 0) {
      write(STDOUT_FILENO, ".", 1);
    }
  }
}

int main() {
  int64_t value = -1;

  // Lock to cpu0
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(0, &mask);
  assert(!sched_setaffinity(0, sizeof(mask), &mask));

  while (1) {
    int fd = start_counter();

    timing_computation();

    ssize_t nread = read(fd, &value, sizeof(value));
    assert(nread == sizeof(value));
    close(fd);

    printf("%lld\n", value);
  }

  return 0;
}
