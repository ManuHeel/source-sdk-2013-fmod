#ifndef ADAPTIVE_MUSIC_SYSTEM_H
#define ADAPTIVE_MUSIC_SYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "cbase.h"
#include "vector"
#include "string"

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
    void CalculateAdaptiveMusicState();

    void ParseKeyValue(KeyValues *keyValue);

    void InitAdaptiveMusic();

    void ShutDownAdaptiveMusic();

    CBasePlayer *SetAdaptiveMusicPlayer();

    // Struct for holding ZoneWatcher zones data
    struct Zone {
        float minOrigin[3];
        float maxOrigin[3];
        const char *parameterName;
        bool lastKnownZoneStatus = false;
    };

    // Struct for holding Scene data
    struct Scene {
        const char *sceneName;
        const char *stateName;
        const char *parameterName;
        bool lastKnownSceneStateStatus = false;
    };

};

extern CAdaptiveMusicSystem *AdaptiveMusicSystem();

#endif //ADAPTIVE_MUSIC_SYSTEM_H