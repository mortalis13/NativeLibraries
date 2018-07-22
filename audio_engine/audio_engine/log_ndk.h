#ifndef LOG_NDK_H
#define LOG_NDK_H

#include "vars.h"


namespace Log {

void d(const string& msg, ...);
void dn(const string& msg, ...);
void dg(const string& msg, ...);
void dgn(const string& msg, ...);

void e(const string& msg, ...);
void eg(const string& msg, ...);
void egn(const string& msg, ...);

void w(const string& msg, ...);
void wg(const string& msg, ...);
void wgn(const string& msg, ...);

}

#endif