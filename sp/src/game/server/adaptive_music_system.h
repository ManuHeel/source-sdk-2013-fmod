#ifndef ADAPTIVE_MUSIC_SYSTEM_H
#define ADAPTIVE_MUSIC_SYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include <KeyValues.h>

class CAdaptiveMusicSystem : public CAutoGameSystem {

public:

    CAdaptiveMusicSystem();

    virtual bool Init();

    virtual void LevelInitPreEntity();

    virtual void LevelInitPostEntity();

    virtual void LevelShutdownPreEntity();

    // Set the available ConVar if we can find adaptive music data for this level
    void CalculateAdaptiveMusicState();

    void ParseKeyValue(KeyValues *keyValue);

    void InitAdaptiveMusic();

    void ShutDownAdaptiveMusic();

};

extern CAdaptiveMusicSystem *AdaptiveMusicSystem();

#endif //ADAPTIVE_MUSIC_SYSTEM_H