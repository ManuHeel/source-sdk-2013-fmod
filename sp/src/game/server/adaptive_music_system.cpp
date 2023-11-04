#include "cbase.h"
#include "adaptive_music_system.h"
#include "adaptive_music_watcher.h"
#include "tier0/icommandline.h"
#include "igamesystem.h"
#include "filesystem.h"
#include <KeyValues.h>
#include <string>
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

class CFMODEventListener : public IGameEventListener2 {

private:
    CAdaptiveMusicSystem *pAdaptiveMusicSystem;

public:
    explicit CFMODEventListener(CAdaptiveMusicSystem *pAdaptiveMusicSystemRef) {
        pAdaptiveMusicSystem = pAdaptiveMusicSystemRef;
        gameeventmanager->AddListener(this, "player_spawn", true);
    }

    //-----------------------------------------------------------------------------
    // Purpose: Triggered when a listened GameEvent is fired
    // Input: The fired GameEvent
    //-----------------------------------------------------------------------------
    void FireGameEvent(IGameEvent *pEvent) override {
        // When the player is spawned, set the pAdaptiveMusicPlayer for future reference and initialize the music
        if (Q_strcmp(pEvent->GetName(), "player_spawn") == 0) {
            Msg("FMOD Adaptive Music - Player spawned, initializing Adaptive Music\n");
            pAdaptiveMusicSystem->SetAdaptiveMusicPlayer();
            pAdaptiveMusicSystem->InitAdaptiveMusic();
        }
    }
};

CFMODEventListener *pFMODEventListener;

const char *loadedBankName;
const char *startedEventPath;

//-----------------------------------------------------------------------------
// Purpose: Get the Adaptive Music Player
// Output: Pointer to a CBasePlayer of the main Player
//-----------------------------------------------------------------------------
CBasePlayer *CAdaptiveMusicSystem::SetAdaptiveMusicPlayer() {
    CBasePlayer *pPlayer;

    if (gpGlobals->maxClients <= 1) {
        pPlayer = UTIL_GetLocalPlayer();
    } else {
        // only respond to the first player
        pPlayer = UTIL_PlayerByIndex(1);
    }

    pAdaptiveMusicPlayer = pPlayer;
    return pPlayer;
}

CAdaptiveMusicSystem::CAdaptiveMusicSystem() : CAutoGameSystem("CAdaptiveMusicSystem") {
}

//-----------------------------------------------------------------------------
// Purpose: Called when the GameSystem is first initialized
// Starts the event listener
//-----------------------------------------------------------------------------
bool CAdaptiveMusicSystem::Init() {
    Msg("FMOD Adaptive Music - Start FMOD event listener\n");
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
    // When loading a map, try to find an existing player (map change) and init the AdaptiveMusic
    Msg("FMOD Adaptive Music - Level loaded, trying to find the player\n");
    SetAdaptiveMusicPlayer();
    if (pAdaptiveMusicPlayer != nullptr) {
        Msg("FMOD Adaptive Music - Player found, starting Adaptive Music\n");
        InitAdaptiveMusic();
    }
    // If the player has not been set, this is probably the game starting instead of a new level loading
    // The AdaptiveMusic will init when the player first spawned (GameEvent)
}

//-----------------------------------------------------------------------------
// Purpose: Called when the Level is shutdown
// Stops the adaptive music for the level
//-----------------------------------------------------------------------------
void CAdaptiveMusicSystem::LevelShutdownPreEntity() {
    ShutDownAdaptiveMusic();
}

void CAdaptiveMusicSystem::Shutdown() {
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
// Input: A KeyValue object, subset of an adaptive music file
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
                loadedBankName = elementValue;
                Log("FMOD Adaptive Music - Telling the FMOD Client to load the bank '%s'\n", loadedBankName);
                // Send a FMODLoadBank UserMessage
                CSingleUserRecipientFilter filter(pAdaptiveMusicPlayer);
                filter.MakeReliable();
                UserMessageBegin(filter, "FMODLoadBank");
                WRITE_STRING(elementValue);
                MessageEnd();
            } else if (!Q_strcmp(elementKey, "event")) {
                startedEventPath = elementValue;
                ShutDownAdaptiveMusic(); // Ensure an existing music is not playing anymore (avoids piling up events)
                Log("FMOD Adaptive Music - Telling the FMOD Client to start the event '%s'\n", startedEventPath);
                // Send a FMODStartEvent UserMessage
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
            // DOING: Init the AdaptiveMusicWatcher according to the params
            if (!Q_strcmp(elementKey, "type")) {
                const char *watcherType = elementValue;
                if (!Q_strcmp(watcherType, "health")) {
                    Msg("FMOD Adaptive Music - Found a new HealthWatcher to create\n");
                    // Spawn the watcher entity
                    CBaseEntity *pNode = CreateEntityByName("adaptive_music_watcher"); // For now, creating a default watcher
                    if (pNode) {
                        DispatchSpawn(pNode);
                        EHANDLE hHandle;
                        hHandle = pNode;
                        auto *pAdaptiveMusicWatcher = dynamic_cast<CAdaptiveMusicWatcher *>(pNode);
                        pAdaptiveMusicWatcher->SetAdaptiveMusicSystem(this);
                        pAdaptiveMusicWatcher->Activate();
                    } else {
                        Warning("FMOD Adaptive Music - Failed to spawn AdaptiveMusicWatcher entity\n");
                    }


                }
            }
            element = element->GetNextKey();
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: Start the AdaptiveMusic up
// Finds the adaptive music file, parse it and initialize everything related to AdaptiveMusic for this level
//-----------------------------------------------------------------------------
void CAdaptiveMusicSystem::InitAdaptiveMusic() {
    // If we know there's Adaptive Music data, initialize the Adaptive Music
    if (adaptive_music_available.GetBool()) {
        Msg("FMOD Adaptive Music - Initializing the map's adaptive music data\n");
        // Find the adaptive music file
        char szFullName[512];
        Q_snprintf(szFullName, sizeof(szFullName), "maps/%s_adaptivemusic.txt", STRING(gpGlobals->mapname));
        auto *keyValue = new KeyValues("adaptive_music");
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
        // Else shutdown the Adaptive Music
    else {
        ShutDownAdaptiveMusic();
    }
}

//-----------------------------------------------------------------------------
// Purpose: Shuts down the adaptive music for the map
//-----------------------------------------------------------------------------
void CAdaptiveMusicSystem::ShutDownAdaptiveMusic() {
    Msg("FMOD Adaptive Music - Shutting down adaptive music for the map\n");
    SetAdaptiveMusicPlayer();
    if (pAdaptiveMusicPlayer != nullptr && startedEventPath != nullptr) {
        Log("FMOD Adaptive Music - Telling the FMOD Client to stop the event '%s'\n", startedEventPath);
        // Send a FMODStopEvent usermessage
        CSingleUserRecipientFilter filter(pAdaptiveMusicPlayer);
        filter.MakeReliable();
        UserMessageBegin(filter, "FMODStopEvent");
        WRITE_STRING(startedEventPath);
        MessageEnd();
    }
}

CAdaptiveMusicSystem g_AdaptiveMusicSystem;
