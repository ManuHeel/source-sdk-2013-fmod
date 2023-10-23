#ifndef ADAPTIVE_MUSIC_WATCHER_H
#define ADAPTIVE_MUSIC_WATCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "igamesystem.h"
#include "adaptive_music_system.h"

class CAdaptiveMusicWatcher : public CAutoGameSystem {

private:

public:
    CAdaptiveMusicWatcher();
    CAdaptiveMusicWatcher(CAdaptiveMusicSystem *adaptiveMusicSystem);

    virtual bool Init();

    virtual void Update();
};

extern CAdaptiveMusicWatcher *AdaptiveMusicWatcher();

#endif //ADAPTIVE_MUSIC_WATCHER_H
