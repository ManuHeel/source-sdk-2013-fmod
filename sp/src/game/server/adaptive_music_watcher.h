#ifndef ADAPTIVE_MUSIC_WATCHER_H
#define ADAPTIVE_MUSIC_WATCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

class CAdaptiveMusicWatcher : public CLogicalEntity {

private:
    CAdaptiveMusicSystem *pAdaptiveMusicSystem {};

public:
    DECLARE_CLASS(CAdaptiveMusicWatcher, CLogicalEntity);
    DECLARE_DATADESC();

    explicit CAdaptiveMusicWatcher() = default;

    void SetAdaptiveMusicSystem(CAdaptiveMusicSystem *pAdaptiveMusicSystemRef);

    void Spawn() override;

    void WatchThink();

private:

};

LINK_ENTITY_TO_CLASS(adaptive_music_watcher, CAdaptiveMusicWatcher);

BEGIN_DATADESC(CAdaptiveMusicWatcher)
                    DEFINE_THINKFUNC(WatchThink)
END_DATADESC()

#endif //ADAPTIVE_MUSIC_WATCHER_H