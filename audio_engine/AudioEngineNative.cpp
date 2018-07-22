#include "AudioEngineNative.h"

#include "audio_engine/SoundManager.h"
#include "audio_engine/wav_reader.h"

using namespace std;

// #define CLASS_NAME_AUDIO_FILE "org/home/metronomics/AudioFile"


SoundManager* mSoundManager;


JNIEXPORT void JNICALL Java_org_home_metronomics_AudioEngineNative_init(JNIEnv *env, jclass obj) {
  mSoundManager = new SoundManager();
  if (mSoundManager->start() != 0) return;
}


JNIEXPORT void JNICALL Java_org_home_metronomics_AudioEngineNative_playSound(JNIEnv *env, jclass obj, jstring jaudioPath) {
  const char* audioPathBytes = env->GetStringUTFChars(jaudioPath, 0);
  string audioPath(audioPathBytes);
  env->ReleaseStringUTFChars(jaudioPath, audioPathBytes);
  
  // -- Get Data
  WavReader wavReader;
  wavReader.readWav(audioPath);
  
  int16_t numChannels = wavReader.getNumChannels();
  uint32_t sampleRate = wavReader.getSampleRate();
  int32_t bytesPerSecond = wavReader.getBytesPerSecond();
  int16_t bytesPerBlock = wavReader.getBytesPerBlock();
  int16_t bitsPerSample = wavReader.getBitsPerSample();
  
  vector<uint8_t> audioData = wavReader.getAudioData();
  int len = audioData.size();
  uint8_t* data = new uint8_t[len];
  memcpy(data, &audioData.front(), len);
  
  Log::d("--Loading audio file: " + audioPath + "--");
  Log::d("numChannels: %d", numChannels);
  Log::d("sampleRate: %d", sampleRate);
  Log::d("bytesPerSecond: %d", bytesPerSecond);
  Log::d("bytesPerBlock: %d", bytesPerBlock);
  Log::d("bitsPerSample: %d", bitsPerSample);
  Log::d("audioData: %d", audioData.size());

  // -- Load and play
  // SoundManager* mSoundManager = new SoundManager();
  // int res = mSoundManager->start();
  // if (res != 0) return;
  mSoundManager->playSound(data, len);
}


JNIEXPORT void JNICALL Java_org_home_metronomics_AudioEngineNative_playPattern(JNIEnv *env, jclass obj) {
  mSoundManager->playPattern();
}

JNIEXPORT void JNICALL Java_org_home_metronomics_AudioEngineNative_stopPattern(JNIEnv *env, jclass obj) {
  mSoundManager->stopPattern();
}
