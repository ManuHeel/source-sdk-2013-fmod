#include "cbase.h"
#include "adaptive_music_system.h"
#include "tier0/icommandline.h"
#include "igamesystem.h"
#include "filesystem.h"
#include <KeyValues.h>
#include "isaverestore.h"
#include "gamestats.h"
#include "ai_basenpc.h"
#include "Sprite.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//===========================================================================================================
// ADAPTIVE MUSIC GAME SYSTEM
//===========================================================================================================

ConVar adaptive_music_available("adaptive_music_available", "0", FCVAR_NONE, "Automatically set by the game when an adaptive music file is available for the current map.");

//-----------------------------------------------------------------------------
// Purpose: In multiplayer, always return player 1
//-----------------------------------------------------------------------------
CBasePlayer *GetAdaptiveMusicPlayer(void) {
    CBasePlayer *pPlayer;

    if (gpGlobals->maxClients <= 1) {
        pPlayer = UTIL_GetLocalPlayer();
    } else {
        // only respond to the first player
        pPlayer = UTIL_PlayerByIndex(1);
    }

    return pPlayer;
}

class CFMODEventListener : public IGameEventListener2 {

private:
    CAdaptiveMusicSystem *pAdaptiveMusicSystem;

public:
    CFMODEventListener(CAdaptiveMusicSystem *pAdaptiveMusicSystemRef) {
        pAdaptiveMusicSystem = pAdaptiveMusicSystemRef;
        gameeventmanager->AddListener(this, "player_spawn", true);
    }

    virtual void FireGameEvent(IGameEvent *pEvent) {
        if (Q_strcmp(pEvent->GetName(), "player_spawn") == 0) {
            pAdaptiveMusicSystem->InitAdaptiveMusic();
        }
    }
};

//-----------------------------------------------------------------------------
// Purpose: Game system to kickstart the adaptive music
//-----------------------------------------------------------------------------

CFMODEventListener *pFMODEventListener;

CAdaptiveMusicSystem::CAdaptiveMusicSystem() : CAutoGameSystem("CAdaptiveMusicSystem") {
}

bool CAdaptiveMusicSystem::Init() {
    return true;
}

void CAdaptiveMusicSystem::LevelInitPreEntity() {
    CalculateAdaptiveMusicState();
}

void CAdaptiveMusicSystem::LevelInitPostEntity() {
    Msg("Start FMOD event listener");
    pFMODEventListener = new CFMODEventListener(this);
    // Don't init right away but wait for the player to be spawned
    // InitAdaptiveMusic();
}

void CAdaptiveMusicSystem::LevelShutdownPreEntity() {
    ShutDownAdaptiveMusic();
}

// Set the available ConVar if we can find adaptive music data for this level
void CAdaptiveMusicSystem::CalculateAdaptiveMusicState() {
    char szFullName[512];
    Q_snprintf(szFullName, sizeof(szFullName), "maps/%s_adaptivemusic.txt", STRING(gpGlobals->mapname));
    if (filesystem->FileExists(szFullName)) {
        Msg("FMOD Adaptive Music - Found adaptive music file, '%s'\n", szFullName);
        adaptive_music_available.SetValue(true);
    } else {
        Msg("FMOD Adaptive Music - Could not find adaptive music file, '%s'\n", szFullName);
        adaptive_music_available.SetValue(false);
    }
}

void CAdaptiveMusicSystem::ParseKeyValue(KeyValues *keyValue) {
    const char *keyValueName = keyValue->GetName();
    Log("FMOD Adaptive Music - Parsing the KeyValue '%s'\n", keyValueName);
    if (!Q_strcmp(keyValueName, "globals")) { // The key-value element is defining the global parameters of the map
        KeyValues *element = keyValue->GetFirstSubKey();
        while (element) {
            const char *elementKey = element->GetName();
            const char *elementValue = element->GetString();
            Log("FMOD Adaptive Music - %s: %s\n", elementKey, elementValue);
            if (!Q_strcmp(elementKey, "bank")) {
                CBasePlayer *pBasePlayer = GetAdaptiveMusicPlayer();
                CSingleUserRecipientFilter filter(pBasePlayer);
                filter.MakeReliable();
                UserMessageBegin(filter, "FMODLoadBank");
                WRITE_STRING(elementValue);
                MessageEnd();
            } else if (!Q_strcmp(elementKey, "event")) {
                CBasePlayer *pBasePlayer = GetAdaptiveMusicPlayer();
                CSingleUserRecipientFilter filter(pBasePlayer);
                filter.MakeReliable();
                UserMessageBegin(filter, "FMODStartEvent");
                WRITE_STRING(elementValue);
                MessageEnd();
            }
            element = element->GetNextKey();
        }
    } else if (!Q_strcmp(keyValueName, "watcher")) { // The key-value element is defining a watcher and its params for the map
        KeyValues *element = keyValue->GetFirstSubKey();
        while (element) {
            const char *elementKey = element->GetName();
            const char *elementValue = element->GetString();
            Log("FMOD Adaptive Music - %s: %s\n", elementKey, elementValue);
            element = element->GetNextKey();
        }
    }
}

void CAdaptiveMusicSystem::InitAdaptiveMusic() {
    Msg("FMOD Adaptive Music - Initializing the map's adaptive music data\n");
    // Find the adaptive music file
    char szFullName[512];
    Q_snprintf(szFullName, sizeof(szFullName), "maps/%s_adaptivemusic.txt", STRING(gpGlobals->mapname));
    KeyValues *keyValue = new KeyValues("adaptive_music");
    if (keyValue->LoadFromFile(filesystem, szFullName, "MOD")) {
        Msg("FMOD Adaptive Music - Loading adaptive music data from '%s'\n", szFullName);
        KeyValues *keyValueSubset = keyValue->GetFirstSubKey();
        while (keyValueSubset) {
            ParseKeyValue(keyValueSubset);
            keyValueSubset = keyValueSubset->GetNextKey();
        }
    } else {
        Msg("FMOD Adaptive Music - Could not load adaptive music file '%s'\n", szFullName);
    }
}

void CAdaptiveMusicSystem::ShutDownAdaptiveMusic() {
    Msg("FMOD Adaptive Music - Shutting down adaptive music for the map\n");
}

CAdaptiveMusicSystem g_AdaptiveMusicSystem;
