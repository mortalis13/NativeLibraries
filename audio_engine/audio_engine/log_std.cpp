
#include "log_std.h"

#include <stdarg.h>

#include "utils.h"


namespace Log {

void d(const string& msg, ...) {
  if (Config::TEST_MODE) return;
  va_list ap;
  va_start(ap, msg);
  string format = msg + "\n";
  vfprintf(stdout, format.c_str(), ap);
  fflush(stdout);
  va_end(ap);
}

void dn(const string& msg, ...) {
  if (Config::TEST_MODE) return;
  va_list ap;
  va_start(ap, msg);
  vfprintf(stdout, msg.c_str(), ap);
  fflush(stdout);
  va_end(ap);
}

void dg(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  string format = msg + "\n";
  vfprintf(stdout, format.c_str(), ap);
  fflush(stdout);
  va_end(ap);
}

void dgn(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  vfprintf(stdout, msg.c_str(), ap);
  fflush(stdout);
  va_end(ap);
}


void e(const string& msg, ...) {
  if (Config::TEST_MODE) return;
  va_list ap;
  va_start(ap, msg);
  string format = msg + "\n";
  vfprintf(stderr, format.c_str(), ap);
  fflush(stderr);
  va_end(ap);
}

void eg(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  string format = msg + "\n";
  vfprintf(stderr, format.c_str(), ap);
  fflush(stderr);
  va_end(ap);
}

void egn(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  vfprintf(stderr, msg.c_str(), ap);
  fflush(stderr);
  va_end(ap);
}


void w(const string& msg, ...) {
  if (Config::TEST_MODE) return;
  va_list ap;
  va_start(ap, msg);
  string format = msg + "\n";
  vfprintf(stderr, format.c_str(), ap);
  fflush(stderr);
  va_end(ap);
}

void wg(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  string format = msg + "\n";
  vfprintf(stderr, format.c_str(), ap);
  fflush(stderr);
  va_end(ap);
}

void wgn(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  vfprintf(stderr, msg.c_str(), ap);
  fflush(stderr);
  va_end(ap);
}

}
