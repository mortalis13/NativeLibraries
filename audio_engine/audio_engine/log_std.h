#ifndef LOG_STD_H
#define LOG_STD_H

#include "vars.h"


namespace Log {

void logStr(const string& msg, bool detectTestMode = true);
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