#ifndef SOUNDMANAGER_HPP
#define SOUNDMANAGER_HPP

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <sys/stat.h>

#include "SoundQueue.h"
#include "utils.h"


struct Sound {
  uint8_t* data;
  size_t len;
  
  Sound() {}
  Sound(uint8_t* _data, size_t _len): data(_data), len(_len) {}
};


class SoundManager {
public:
    SoundManager();
    ~SoundManager();

    int start();
    void stop();
    
    void playSound(uint8_t* data, size_t len);
    void playPattern();
    void stopPattern();

private:
    SoundManager(const SoundManager&);
    void operator=(const SoundManager&);
    
    void loadSounds();
    int getBeatSize(int bytesPerSecond, int bpm);
    void updateSilenceBuf(int size);
    SoundQueue& nextQueue();
    
    // OpenSL ES engine.
    SLObjectItf mEngineObj;
    SLEngineItf mEngine;
    
    // Audio output.
    SLObjectItf mOutputMixObj;
    
    // Sound players.
    static const int32_t QUEUE_COUNT = 20;
    SoundQueue mSoundQueues[QUEUE_COUNT];
    
    SoundQueue kickQueue1;
    SoundQueue kickQueue2;
    SoundQueue hihatQueue1;
    SoundQueue hihatQueue2;
    SoundQueue loopQueue;
    
    int32_t mCurrentQueue;
    
    Sound* kickSound;
    Sound* hihatSound;
    
    vector<Sound*> soundData;
    uint8_t* silenceBuf;
    uint8_t* loopSilence;
    
    int beatSize;
    bool playerStop;
};
#endif
