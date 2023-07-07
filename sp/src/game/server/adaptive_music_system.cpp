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

CBasePlayer *pAdaptiveMusicPlayer;

//-----------------------------------------------------------------------------
// Purpose: Get the Adaptive Music Player
// Output: Pointer to a CBasePlayer of the main Player
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

    //-----------------------------------------------------------------------------
    // Purpose: Triggered when a listened GameEvent is fired
    // Input: The fired GameEvent
    //-----------------------------------------------------------------------------
    virtual void FireGameEvent(IGameEvent *pEvent) {
        if (Q_strcmp(pEvent->GetName(), "player_spawn") == 0) {
            pAdaptiveMusicPlayer = GetAdaptiveMusicPlayer();
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

//-----------------------------------------------------------------------------
// Purpose: Called when the GameSystem is first initialized
// Starts the event listener
//-----------------------------------------------------------------------------
bool CAdaptiveMusicSystem::Init() {
    Msg("FMOD Adaptive Music - Start FMOD event listener");
    pFMODEventListener = new CFMODEventListener(this);
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called when the Level is initialized
// Checks if the level has adaptive music data
//-----------------------------------------------------------------------------
void CAdaptiveMusicSystem::LevelInitPreEntity() {
    CalculateAdaptiveMusicState();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the Level is initialized
// Tries to initialize the adaptive music if the player has been found
//-----------------------------------------------------------------------------
void CAdaptiveMusicSystem::LevelInitPostEntity() {
    // Init only when the player has already been found (map change)
    if (pAdaptiveMusicPlayer != NULL) {
        InitAdaptiveMusic();
    }
}

//-----------------------------------------------------------------------------
// Purpose: Called when the Level is shutdown
// Stops the adaptive music for the level
//-----------------------------------------------------------------------------
void CAdaptiveMusicSystem::LevelShutdownPreEntity() {
    ShutDownAdaptiveMusic();
}

//-----------------------------------------------------------------------------
// Purpose: Checks if the level has adaptive music data
// Sets the available ConVar if we can find adaptive music data for this level
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose: Parses the provided KeyValue containing adaptive music data from a file
// Input: A KeyValue object, subset of a adaptive music file
//-----------------------------------------------------------------------------
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
                CSingleUserRecipientFilter filter(pAdaptiveMusicPlayer);
                filter.MakeReliable();
                UserMessageBegin(filter, "FMODLoadBank");
                WRITE_STRING(elementValue);
                MessageEnd();
            } else if (!Q_strcmp(elementKey, "event")) {
                CSingleUserRecipientFilter filter(pAdaptiveMusicPlayer);
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
            // TODO: Init the AdaptiveMusicWatcher according to the params
            element = element->GetNextKey();
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: Start the AdaptiveMusic up
// Finds the adaptive music file, parse it and initialize everything related to AdaptiveMusic for this level
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose: Shuts down the adaptive music for the map
// TODO : Stop the events (or not, if we want to have music on loading times)
//-----------------------------------------------------------------------------
void CAdaptiveMusicSystem::ShutDownAdaptiveMusic() {
    Msg("FMOD Adaptive Music - Shutting down adaptive music for the map\n");
}

CAdaptiveMusicSystem g_AdaptiveMusicSystem;
