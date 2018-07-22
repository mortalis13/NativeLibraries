#include "WavReaderNative.h"

#include "audio_engine/wav_reader.h"

using namespace std;

#define CLASS_NAME_AUDIO_FILE "org/home/metronomics/AudioFile"


JNIEXPORT jobject JNICALL Java_org_home_metronomics_WavReaderNative_getAudioData(JNIEnv *env, jclass obj, jstring jaudioPath) {
  const char* audioPathBytes = env->GetStringUTFChars(jaudioPath, 0);
  string audioPath(audioPathBytes);
  env->ReleaseStringUTFChars(jaudioPath, audioPathBytes);
  
  // -- process
  WavReader wavReader;
  wavReader.readWav(audioPath);
  
  int16_t numChannels = wavReader.getNumChannels();
  uint32_t sampleRate = wavReader.getSampleRate();
  int32_t bytesPerSecond = wavReader.getBytesPerSecond();
  int16_t bytesPerBlock = wavReader.getBytesPerBlock();
  int16_t bitsPerSample = wavReader.getBitsPerSample();
  
  vector<uint8_t> audioData = wavReader.getAudioData();
  
  Log::d("--Loading audio file: " + audioPath + "--");
  
  Log::d("numChannels: %d", numChannels);
  Log::d("sampleRate: %d", sampleRate);
  Log::d("bytesPerSecond: %d", bytesPerSecond);
  Log::d("bytesPerBlock: %d", bytesPerBlock);
  Log::d("bitsPerSample: %d", bitsPerSample);
  
  Log::d("audioData: %d", audioData.size());
  
  // -- result
  jclass objectCls = env->FindClass(CLASS_NAME_AUDIO_FILE);
  if (objectCls == NULL) {
    Log::e("Could not find class for '%s'", CLASS_NAME_AUDIO_FILE);
    return NULL;
  }
  
  jmethodID objectConstructor = env->GetMethodID(objectCls, "<init>", "()V");
  jobject result = env->NewObject(objectCls, objectConstructor);
  if (result == NULL) {
    Log::e("Could not create object for '%s'", CLASS_NAME_AUDIO_FILE);
    return NULL;
  }
  
  size_t len = audioData.size();
  jbyteArray audioDataBytes = env->NewByteArray(len);
  jbyte* pByteArray = env->GetByteArrayElements(audioDataBytes, NULL);
  memcpy(pByteArray, (char*) audioData.data(), len);
  env->ReleaseByteArrayElements(audioDataBytes, pByteArray, 0);
  
  jfieldID audioDataId = env->GetFieldID(objectCls, "audioData", "[B");
  env->SetObjectField(result, audioDataId, audioDataBytes);
  
  jint numChannelsVal = numChannels;
  jfieldID numChannelsId = env->GetFieldID(objectCls, "numChannels", "I");
  env->SetIntField(result, numChannelsId, numChannelsVal);
  
  jint sampleRateVal = sampleRate;
  jfieldID sampleRateId = env->GetFieldID(objectCls, "sampleRate", "I");
  env->SetIntField(result, sampleRateId, sampleRateVal);
  
  jint bytesPerSecondVal = bytesPerSecond;
  jfieldID bytesPerSecondId = env->GetFieldID(objectCls, "bytesPerSecond", "I");
  env->SetIntField(result, bytesPerSecondId, bytesPerSecondVal);
  
  jint bytesPerBlockVal = bytesPerBlock;
  jfieldID bytesPerBlockId = env->GetFieldID(objectCls, "bytesPerBlock", "I");
  env->SetIntField(result, bytesPerBlockId, bytesPerBlockVal);
  
  jint bitsPerSampleVal = bitsPerSample;
  jfieldID bitsPerSampleId = env->GetFieldID(objectCls, "bitsPerSample", "I");
  env->SetIntField(result, bitsPerSampleId, bitsPerSampleVal);
  
  return result;
}


// JNIEXPORT jbyteArray JNICALL Java_org_home_metronomics_WavReaderNative_getAudioData(JNIEnv *env, jclass obj, jstring jaudioPath) {
//   const char* audioPathBytes = env->GetStringUTFChars(jaudioPath, 0);
//   string audioPath(audioPathBytes);
//   env->ReleaseStringUTFChars(jaudioPath, audioPathBytes);
  
//   // -- process
//   WavReader wavReader;
//   wavReader.readWav(audioPath);
  
//   int16_t numChannels = wavReader.getNumChannels();
//   uint32_t sampleRate = wavReader.getSampleRate();
//   int32_t bytesPerSecond = wavReader.getBytesPerSecond();
//   int16_t bytesPerBlock = wavReader.getBytesPerBlock();
//   int16_t bitsPerSample = wavReader.getBitsPerSample();
  
//   vector<uint8_t> audioData = wavReader.getAudioData();
  
//   Log::d("--Loading audio file: " + audioPath + "--");
  
//   Log::d("numChannels: %d", numChannels);
//   Log::d("sampleRate: %d", sampleRate);
//   Log::d("bytesPerSecond: %d", bytesPerSecond);
//   Log::d("bytesPerBlock: %d", bytesPerBlock);
//   Log::d("bitsPerSample: %d", bitsPerSample);
  
//   Log::d("audioData: %d", audioData.size());
  
//   // -- result
//   size_t len = audioData.size();
//   jbyteArray result = env->NewByteArray(len);
//   jbyte* pByteArray = env->GetByteArrayElements(result, NULL);
//   memcpy(pByteArray, (char*) audioData.data(), len);
//   env->ReleaseByteArrayElements(result, pByteArray, 0);
  
//   return result;
// }
