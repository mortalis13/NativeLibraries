#ifndef SOUNDQUEUE_HPP
#define SOUNDQUEUE_HPP

#include <sys/stat.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// #include "SoundManager.h"
class SoundManager;

class SoundQueue {
public:
    SoundQueue();

    int initialize(SLEngineItf pEngine, SLObjectItf pOutputMixObj, int numBufs);
    int initialize(SLEngineItf pEngine, SLObjectItf pOutputMixObj, int numBufs, SoundManager* manager);
    void finalize();
    void playSound(uint8_t* data, size_t len);
    void playSilence(size_t len);
    void clear();
    
    // void registerCallback(slBufferQueueCallback callback);
    void registerCallback(SoundManager* manager);

private:
    SoundQueue(const SoundQueue&);
    // void operator=(const SoundQueue&);

    // Sound player.
    SLObjectItf mPlayerObj;
    SLPlayItf mPlayer;
    SLBufferQueueItf mPlayerQueue;
};
#endif
