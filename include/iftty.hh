#ifndef IVANP_IFTTY_HH
#define IVANP_IFTTY_HH

#include <unistd.h>
#include <iostream>
#include <cstdio>

class iftty {
  const char* str;

public:
  iftty(const char* str, int fd=1): str(::isatty(fd) ? str : nullptr) { }

  friend std::ostream& operator<<(std::ostream& s, iftty x) {
    if (x.str) s << x.str;
    return s;
  }
};

#endif
