#include "SoundManager.h"

#include <math.h>

#include "wav_reader.h"

SoundManager::SoundManager() :
    mEngineObj(NULL), mEngine(NULL), mOutputMixObj(NULL),
    mSoundQueues(), mCurrentQueue(0),
    silenceBuf(NULL), playerStop(false)
{
    Log::d("Creating SoundManager.");
}

SoundManager::~SoundManager() {
    Log::d("Destroying SoundManager.");
}

int SoundManager::start() {
    Log::d("Starting SoundManager.");
    
    SLresult result;
    const SLuint32      engineMixIIDCount = 1;
    const SLInterfaceID engineMixIIDs[]   = {SL_IID_ENGINE};
    const SLboolean     engineMixReqs[]   = {SL_BOOLEAN_TRUE};
    
    const SLuint32      outputMixIIDCount = 0;
    const SLInterfaceID outputMixIIDs[]   = {};
    const SLboolean     outputMixReqs[]   = {};

    // Creates OpenSL ES engine and dumps its capabilities.
    result = slCreateEngine(&mEngineObj, 0, NULL, engineMixIIDCount, engineMixIIDs, engineMixReqs);
    if (result != SL_RESULT_SUCCESS) goto ERROR;
    result = (*mEngineObj)->Realize(mEngineObj,SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) goto ERROR;
    result = (*mEngineObj)->GetInterface(mEngineObj, SL_IID_ENGINE, &mEngine);
    if (result != SL_RESULT_SUCCESS) goto ERROR;

    // Creates audio output.
    result = (*mEngine)->CreateOutputMix(mEngine, &mOutputMixObj, outputMixIIDCount, outputMixIIDs, outputMixReqs);
    result = (*mOutputMixObj)->Realize(mOutputMixObj, SL_BOOLEAN_FALSE);
    
    // if (loopQueue.initialize(mEngine, mOutputMixObj, 20, this) != 0) goto ERROR;
    // if (kickQueue1.initialize(mEngine, mOutputMixObj, 5) != 0) goto ERROR;
    // if (kickQueue2.initialize(mEngine, mOutputMixObj, 5) != 0) goto ERROR;
    // if (hihatQueue1.initialize(mEngine, mOutputMixObj, 5) != 0) goto ERROR;
    // if (hihatQueue2.initialize(mEngine, mOutputMixObj, 5) != 0) goto ERROR;
    
    if (kickQueue1.initialize(mEngine, mOutputMixObj, 5, this) != 0) goto ERROR;
    
    // Set-up sound player.
    // for (int32_t i= 0; i < QUEUE_COUNT; ++i) {
    //   if (mSoundQueues[i].initialize(mEngine, mOutputMixObj, 2) != 0) goto ERROR;
    // }
    
    loadSounds();
    
    return 0;

ERROR:
    Log::e("Error while starting SoundManager");
    stop();
    return 1;
}

void SoundManager::loadSounds() {
  Log::d("loadSounds()");
  
  string appDir = "/sdcard/Metronomics/";
  string sounds[] = {
    "ez_kick.wav",
    "ez_snare.wav",
    "ez_hihat_closed.wav",
    "ez_hihat_open.wav",
  };
  
  for (string audioPath: sounds) {
    WavReader wavReader;
    wavReader.readWav(appDir + audioPath);
    
    vector<uint8_t> audioData = wavReader.getAudioData();
    int len = audioData.size();
    uint8_t* data = new uint8_t[len];
    memcpy(data, &audioData.front(), len);
    
    Sound* sound = new Sound(data, len);
    soundData.push_back(sound);
  }
  
  kickSound = soundData.at(0);
  hihatSound = soundData.at(2);
  
  beatSize = getBeatSize(88200, 60);
  loopSilence = new uint8_t[4*beatSize]();
  updateSilenceBuf(4*beatSize);
}

void SoundManager::stop() {
    Log::d("Stopping SoundManager.");
    
    // Destroys sound player.
    for (int32_t i= 0; i < QUEUE_COUNT; ++i) {
        mSoundQueues[i].finalize();
    }
    
    // Destroys audio output and engine.
    if (mOutputMixObj != NULL) {
        (*mOutputMixObj)->Destroy(mOutputMixObj);
        mOutputMixObj = NULL;
    }
    if (mEngineObj != NULL) {
        (*mEngineObj)->Destroy(mEngineObj);
        mEngineObj = NULL;
        mEngine = NULL;
    }
}


void SoundManager::playSound(uint8_t* data, size_t len) {
  int32_t currentQueue = ++mCurrentQueue;
  SoundQueue& soundQueue = mSoundQueues[currentQueue % QUEUE_COUNT];
  soundQueue.playSound(data, len);
}


int SoundManager::getBeatSize(int bytesPerSecond, int bpm) {
  return bytesPerSecond * 60 / bpm;
}

void SoundManager::updateSilenceBuf(int size) {
  Log::d("updateSilenceBuf()");
  
  if (silenceBuf != NULL) delete[] silenceBuf;
  silenceBuf = new uint8_t[size]();
}

SoundQueue& SoundManager::nextQueue() {
  SoundQueue& soundQueue = mSoundQueues[mCurrentQueue++];
  if (mCurrentQueue >= QUEUE_COUNT) {
    Log::d("Resetting Queue index");
    mCurrentQueue = 0;
  }
  return soundQueue;
}

void SoundManager::playPattern() {
  Log::d("playPattern()");
  int len = 88200;
  kickQueue1.playSound(kickSound->data, len);
}

// void SoundManager::playPattern() {
//   Log::d("playPattern()");
//   Sound* sound;
//   SoundQueue soundQueue;
  
//   loopQueue.playSound(loopSilence, 2*beatSize);
  
//   soundQueue = nextQueue();
//   soundQueue.playSound(kickSound->data, kickSound->len);
  
//   soundQueue = nextQueue();
//   soundQueue.playSound(silenceBuf, beatSize);
//   soundQueue.playSound(hihatSound->data, hihatSound->len);
  
//   // soundQueue = nextQueue();
//   // soundQueue.playSound(silenceBuf, 2*beatSize);
//   // soundQueue.playSound(kickSound->data, kickSound->len);
  
//   // soundQueue = nextQueue();
//   // soundQueue.playSound(silenceBuf, 3*beatSize);
//   // soundQueue.playSound(hihatSound->data, hihatSound->len);
// }


// void SoundManager::playPattern() {
//   Log::d("playPattern()");
//   Sound* sound;
  
//   // loopQueue.clear();
//   // kickQueue1.clear();
//   // kickQueue2.clear();
//   // hihatQueue1.clear();
//   // hihatQueue2.clear();
  
//   loopQueue.playSound(loopSilence, 4*beatSize);
  
//   kickQueue1.playSound(kickSound->data, kickSound->len);
  
//   hihatQueue1.playSound(silenceBuf, beatSize);
//   hihatQueue1.playSound(hihatSound->data, hihatSound->len);
  
//   kickQueue2.playSound(silenceBuf, 2*beatSize);
//   kickQueue2.playSound(kickSound->data, kickSound->len);
  
//   hihatQueue2.playSound(silenceBuf, 3*beatSize);
//   hihatQueue2.playSound(hihatSound->data, hihatSound->len);
// }

void SoundManager::stopPattern() {
  Log::d("stopPattern()");
  
  playerStop = true;
}
