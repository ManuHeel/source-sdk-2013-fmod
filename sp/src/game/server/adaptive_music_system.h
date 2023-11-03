#ifndef ADAPTIVE_MUSIC_SYSTEM_H
#define ADAPTIVE_MUSIC_SYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "cbase.h"

class CAdaptiveMusicSystem : public CAutoGameSystem {

private:
    const char *loadedBankName{};
    const char *startedEventPath{};

public:
    CBasePlayer *pAdaptiveMusicPlayer{};

    CAdaptiveMusicSystem();

    bool Init() override;

    void LevelInitPreEntity() override;

    void LevelInitPostEntity() override;

    void LevelShutdownPreEntity() override;

    void Shutdown() override;

    // Set the available ConVar if we can find adaptive music data for this level
    static void CalculateAdaptiveMusicState();

    void ParseKeyValue(KeyValues *keyValue);

    void InitAdaptiveMusic();

    void ShutDownAdaptiveMusic();

    CBasePlayer *SetAdaptiveMusicPlayer();
};

extern CAdaptiveMusicSystem *AdaptiveMusicSystem();

#endif //ADAPTIVE_MUSIC_SYSTEM_H