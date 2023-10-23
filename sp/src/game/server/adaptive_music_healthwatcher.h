#ifndef ADAPTIVE_MUSIC_HEALTHWATCHER_H
#define ADAPTIVE_MUSIC_HEALTHWATCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "adaptive_music_system.h"
#include "adaptive_music_watcher.h"

class CAdaptiveMusicHealthWatcher : public CAdaptiveMusicWatcher {

private:

public:
    CAdaptiveMusicHealthWatcher();
    CAdaptiveMusicHealthWatcher(CAdaptiveMusicSystem *adaptiveMusicSystem);

    virtual bool Init();

    virtual void Update();
};

extern CAdaptiveMusicHealthWatcher *AdaptiveMusicHealthWatcher();

#endif //ADAPTIVE_MUSIC_HEALTHWATCHER_H
