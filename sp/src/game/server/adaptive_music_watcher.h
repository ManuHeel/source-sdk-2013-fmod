#ifndef ADAPTIVE_MUSIC_WATCHER_H
#define ADAPTIVE_MUSIC_WATCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

class CAdaptiveMusicWatcher : public CLogicalEntity {

private:
    CAdaptiveMusicSystem *pAdaptiveMusicSystem{};
    CBasePlayer *pAdaptiveMusicPlayer{};

    // Health watcher
    bool watchHealth = false;
    float lastKnownHealth = 100.0f;

public:
    DECLARE_CLASS(CAdaptiveMusicWatcher, CLogicalEntity);
    DECLARE_DATADESC();

    explicit CAdaptiveMusicWatcher();

    void SetAdaptiveMusicSystem(CAdaptiveMusicSystem *pAdaptiveMusicSystemRef);

    void Spawn() override;

    void WatchThink();

    void WatchHealth();

private:

};

#endif //ADAPTIVE_MUSIC_WATCHER_H