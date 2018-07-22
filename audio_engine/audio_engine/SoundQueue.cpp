#include "SoundQueue.h"

#include <math.h>

#include "utils.h"

#include "SoundManager.h"

SoundQueue::SoundQueue() :
    mPlayerObj(NULL), mPlayer(NULL), mPlayerQueue()
{
}

int SoundQueue::initialize(SLEngineItf pEngine, SLObjectItf pOutputMixObj, int numBufs) {
  initialize(pEngine, pOutputMixObj, numBufs, NULL);
}

int SoundQueue::initialize(SLEngineItf pEngine, SLObjectItf pOutputMixObj, int numBufs, SoundManager* manager) {
    Log::d("Starting sound player.");
    SLresult result;

    // Set-up sound audio source.
    SLDataLocator_AndroidSimpleBufferQueue dataLocatorIn;
    dataLocatorIn.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    dataLocatorIn.numBuffers = numBufs;
    
    SLDataFormat_PCM dataFormat;
    dataFormat.formatType = SL_DATAFORMAT_PCM;
    dataFormat.numChannels = 1; // Mono sound.
    dataFormat.samplesPerSec = SL_SAMPLINGRATE_44_1;
    dataFormat.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    dataFormat.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    dataFormat.channelMask = SL_SPEAKER_FRONT_CENTER;
    dataFormat.endianness = SL_BYTEORDER_LITTLEENDIAN;

    SLDataSource dataSource;
    dataSource.pLocator = &dataLocatorIn;
    dataSource.pFormat = &dataFormat;

    SLDataLocator_OutputMix dataLocatorOut;
    dataLocatorOut.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    dataLocatorOut.outputMix = pOutputMixObj;

    SLDataSink dataSink;
    dataSink.pLocator = &dataLocatorOut;
    dataSink.pFormat = NULL;

    // Creates the sounds player and retrieves its interfaces.
    const SLuint32 soundPlayerIIDCount = 2;
    const SLInterfaceID soundPlayerIIDs[] = { SL_IID_PLAY, SL_IID_BUFFERQUEUE };
    const SLboolean soundPlayerReqs[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

    result = (*pEngine)->CreateAudioPlayer(pEngine, &mPlayerObj, &dataSource, &dataSink, soundPlayerIIDCount, soundPlayerIIDs, soundPlayerReqs);
    if (result != SL_RESULT_SUCCESS) goto ERROR;
    result = (*mPlayerObj)->Realize(mPlayerObj, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) goto ERROR;

    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_PLAY, &mPlayer);
    if (result != SL_RESULT_SUCCESS) goto ERROR;
    result = (*mPlayerObj)->GetInterface(mPlayerObj, SL_IID_BUFFERQUEUE, &mPlayerQueue);
    if (result != SL_RESULT_SUCCESS) goto ERROR;
    
    if (manager != NULL) {
      registerCallback(manager);
    }
    
    // Starts the sound player. Nothing can be heard while the
    // sound queue remains empty.
    result = (*mPlayer)->SetPlayState(mPlayer, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS) goto ERROR;
    
    return 0;

ERROR:
    Log::e("Error while starting SoundQueue");
    return 1;
}

void SoundQueue::finalize() {
    Log::d("Stopping SoundQueue.");

    // Destroys sound player.
    if (mPlayerObj != NULL) {
        (*mPlayerObj)->Destroy(mPlayerObj);
        mPlayerObj = NULL;
        mPlayer = NULL;
        mPlayerQueue = NULL;
    }
}

void genSignal() {
  int lenTemp = 100000;
  int16_t* bufferTemp = new int16_t[lenTemp];
  
  float angle = 0;
  float frequency = 440;
  for (int i = 0; i < lenTemp; i++) {
    bufferTemp[i] = (int16_t) (sin(angle) * 32767);
    angle += 2*M_PI * frequency / 44100;
  }
}

void SoundQueue::playSound(uint8_t* data, size_t len) {
    // Log::d("playSound()");
    SLresult result;
    SLuint32 playerState;
    
    (*mPlayerObj)->GetState(mPlayerObj, &playerState);
    if (playerState == SL_OBJECT_STATE_REALIZED) {
        int16_t* buffer = (int16_t*) data;

        // Removes any sound from the queue.
        // result = (*mPlayerQueue)->Clear(mPlayerQueue);
        // if (result != SL_RESULT_SUCCESS) goto ERROR;
        
        // Utils::dumpBytesAsStr(data, len);
        // Log::d("len: %d", len);
        
        result = (*mPlayerQueue)->Enqueue(mPlayerQueue, data, len);
        if (result != SL_RESULT_SUCCESS) goto ERROR;
    }
    else {
      Log::e("playerState != SL_OBJECT_STATE_REALIZED");
    }
    
    return;

ERROR:
    Log::e("Error trying to play sound");
}


void SoundQueue::playSilence(size_t len) {
    SLresult result;
    SLuint32 playerState;
    
    (*mPlayerObj)->GetState(mPlayerObj, &playerState);
    if (playerState == SL_OBJECT_STATE_REALIZED) {
        uint8_t* data = new uint8_t[len]();
        int16_t* buffer = (int16_t*) data;

        result = (*mPlayerQueue)->Enqueue(mPlayerQueue, data, len);
        if (result != SL_RESULT_SUCCESS) goto ERROR;
    }
    else {
      Log::e("playerState != SL_OBJECT_STATE_REALIZED");
    }
    
    return;

ERROR:
    Log::e("Error trying to play sound");
}


void SoundQueue::clear() {
  SLresult result;
  SLuint32 playerState;
    
  (*mPlayerObj)->GetState(mPlayerObj, &playerState);
  if (playerState == SL_OBJECT_STATE_REALIZED) {
    // Removes any sound from the queue.
    result = (*mPlayerQueue)->Clear(mPlayerQueue);
    if (result != SL_RESULT_SUCCESS) Log::e("Error trying to clear queue");
  }
}

void loopEndCallback(SLBufferQueueItf caller, void *pContext) {
  Log::d("loopEndCallback()");
  SoundManager& manager = *(SoundManager*) pContext;
  if (pContext == NULL) {
    Log::e("manager NULL");
    return;
  }
  
  manager.playPattern();
}

void SoundQueue::registerCallback(SoundManager* manager) {
  SLresult result;
  result = (*mPlayerQueue)->RegisterCallback(mPlayerQueue, loopEndCallback, manager);
  if (result != SL_RESULT_SUCCESS) Log::e("Error RegisterCallback");
  result = (*mPlayer)->SetCallbackEventsMask(mPlayer, SL_PLAYEVENT_HEADATEND);
  if (result != SL_RESULT_SUCCESS) Log::e("Error SetCallbackEventMask");
}
