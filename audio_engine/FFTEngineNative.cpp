#include "FFTEngineNative.h"

#include <math.h>
#include <fftw3.h>

#include "audio_engine/utils.h"

using namespace std;


JNIEXPORT void JNICALL Java_org_home_metronomics_FFTEngineNative_getFFT(JNIEnv *env, jclass obj, jdoubleArray jdata) {
  // --- data
  int size = env->GetArrayLength(jdata);
  jboolean isCopy;
  jdouble* jdataPtr = env->GetDoubleArrayElements(jdata, &isCopy);

  
  // --- process
  fftw_complex *in, *out;
  fftw_plan p;
  
  int N = size;
  
  in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
  out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
  p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
  
  for (int i = 0; i < N; i++) {
    double sample = jdataPtr[i];
    in[i][0] = sample;
    in[i][1] = 0;
  }
  
  fftw_execute(p);
  
  for (int i = 0; i < N; i++) {
    jdataPtr[i] = (double) sqrtf(out[i][0]*out[i][0] + out[i][1]*out[i][1]);
  }
  
  env->ReleaseDoubleArrayElements(jdata, jdataPtr, 0);
  
  fftw_destroy_plan(p);
  fftw_free(in);
  fftw_free(out);
}
