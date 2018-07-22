
#include "log_ndk.h"

#include <stdarg.h>

#include "utils.h"

#include <android/log.h>
#define DPE_CPP_LOG_TAG "metronomics_cpp"


namespace Log {

void d(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  __android_log_vprint(ANDROID_LOG_DEBUG, DPE_CPP_LOG_TAG, msg.c_str(), ap);
  va_end(ap);
}

void dn(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  __android_log_vprint(ANDROID_LOG_DEBUG, DPE_CPP_LOG_TAG, msg.c_str(), ap);
  va_end(ap);
}

void dg(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  __android_log_vprint(ANDROID_LOG_DEBUG, DPE_CPP_LOG_TAG, msg.c_str(), ap);
  va_end(ap);
}

void dgn(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  __android_log_vprint(ANDROID_LOG_DEBUG, DPE_CPP_LOG_TAG, msg.c_str(), ap);
  va_end(ap);
}


void e(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  __android_log_vprint(ANDROID_LOG_ERROR, DPE_CPP_LOG_TAG, msg.c_str(), ap);
  va_end(ap);
}

void eg(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  __android_log_vprint(ANDROID_LOG_ERROR, DPE_CPP_LOG_TAG, msg.c_str(), ap);
  va_end(ap);
}

void egn(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  __android_log_vprint(ANDROID_LOG_ERROR, DPE_CPP_LOG_TAG, msg.c_str(), ap);
  va_end(ap);
}


void w(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  __android_log_vprint(ANDROID_LOG_WARN, DPE_CPP_LOG_TAG, msg.c_str(), ap);
  va_end(ap);
}

void wg(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  __android_log_vprint(ANDROID_LOG_WARN, DPE_CPP_LOG_TAG, msg.c_str(), ap);
  va_end(ap);
}

void wgn(const string& msg, ...) {
  va_list ap;
  va_start(ap, msg);
  __android_log_vprint(ANDROID_LOG_WARN, DPE_CPP_LOG_TAG, msg.c_str(), ap);
  va_end(ap);
}

}
