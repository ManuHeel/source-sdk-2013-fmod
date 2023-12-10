#include "cbase.h"
#include "adaptive_music_system.h"
#include "adaptive_music_watcher.h"
#include "tier0/icommandline.h"
#include "igamesystem.h"
#include "filesystem.h"
#include <KeyValues.h>
#include <string>
#include <list>
#include "isaverestore.h"
#include "gamestats.h"
#include "ai_basenpc.h"
#include "Sprite.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//===========================================================================================================
// ADAPTIVE MUSIC GAME SYSTEM
//===========================================================================================================

ConVar adaptive_music_available("adaptive_music_available", "0", FCVAR_NONE,
                                "Automatically set by the game when an adaptive music file is available for the current map.");

class CAdaptiveMusicEventListener : public IGameEventListener2 {
private:
    CAdaptiveMusicSystem* pAdaptiveMusicSystem;

public:
    explicit CAdaptiveMusicEventListener(CAdaptiveMusicSystem* pAdaptiveMusicSystemRef) {
        pAdaptiveMusicSystem = pAdaptiveMusicSystemRef;
        gameeventmanager->AddListener(this, "player_spawn", true);
    }

    //-----------------------------------------------------------------------------
    // Purpose: Triggered when a listened GameEvent is fired
    // Input: The fired GameEvent
    //-----------------------------------------------------------------------------
    void FireGameEvent(IGameEvent* pEvent) override {
        // When the player is spawned, set the pAdaptiveMusicPlayer for future reference and initialize the music
        if (Q_strcmp(pEvent->GetName(), "player_spawn") == 0) {
            Msg("FMOD Adaptive Music - Player spawned, initializing Adaptive Music\n");
            pAdaptiveMusicSystem->SetAdaptiveMusicPlayer();
            pAdaptiveMusicSystem->InitAdaptiveMusic();
        }
    }
};

CAdaptiveMusicEventListener* pAdaptiveMusicEventListener;

const char* loadedBankName;
const char* startedEventPath;

//-----------------------------------------------------------------------------
// Purpose: Get the Adaptive Music Player
// Output: Pointer to a CBasePlayer of the main Player
//-----------------------------------------------------------------------------
CBasePlayer* CAdaptiveMusicSystem::SetAdaptiveMusicPlayer() {
    CBasePlayer* pPlayer;

    if (gpGlobals->maxClients <= 1) {
        pPlayer = UTIL_GetLocalPlayer();
    }
    else {
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
    pAdaptiveMusicEventListener = new CAdaptiveMusicEventListener(this);
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
    //ShutDownAdaptiveMusic();
}

void CAdaptiveMusicSystem::Shutdown() {
    //ShutDownAdaptiveMusic();
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
    }
    else {
        Msg("FMOD Adaptive Music - Could not find adaptive music file, '%s'\n", szFullName);
        adaptive_music_available.SetValue(false);
        ShutDownAdaptiveMusic();
    }
}

//-----------------------------------------------------------------------------
// Purpose: Parses the provided KeyValue containing adaptive music data from a file
// Input: A KeyValue object, subset of an adaptive music file
//-----------------------------------------------------------------------------
void CAdaptiveMusicSystem::ParseKeyValue(KeyValues* keyValue) {
    const char* keyValueName = keyValue->GetName();
    Log("FMOD Adaptive Music - Parsing the KeyValue '%s'\n", keyValueName);
    if (!Q_strcmp(keyValueName, "globals")) {
        // The key-value element is defining the global parameters of the map
        KeyValues* element = keyValue->GetFirstSubKey();
        while (element) {
            const char* elementKey = element->GetName();
            const char* elementValue = element->GetString();
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
            }
            else if (!Q_strcmp(elementKey, "event")) {
                startedEventPath = elementValue;
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
    }
    else if (!Q_strcmp(keyValueName, "watcher")) {
        // The key-value element is defining a watcher and its params for the map
        KeyValues* element = keyValue->GetFirstSubKey();

        // Watcher settings
        const char* watcherType = nullptr;
        const char* watcherParam = nullptr;
        const char* watcherEntityClass = nullptr;
        const char* watcherEntityWatchedStatus = nullptr;
        auto* watcherZones = new std::list<Zone>();

        // Step 1: parse all elements to gather the watcher settings
        while (element) {
            const char* elementKey = element->GetName();
            const char* elementValue = element->GetString();
            if (!Q_strcmp(elementKey, "type")) {
                watcherType = elementValue;
            }
            else if (!Q_strcmp(elementKey, "parameter")) {
                watcherParam = elementValue;
            }
            else if (!Q_strcmp(elementKey, "entity_class")) {
                watcherEntityClass = elementValue;
            }
            else if (!Q_strcmp(elementKey, "entity_watched_status")) {
                watcherEntityWatchedStatus = elementValue;
            }
            else if (!Q_strcmp(elementKey, "zones")) {
                // Zone list settings
                KeyValues* zone = element->GetFirstSubKey();
                while (zone) {
                    const char* zoneKey = zone->GetName();
                    if (!Q_strcmp(zoneKey, "zone")) {
                        KeyValues* zoneElement = zone->GetFirstSubKey();
                        // Create a new Zone struct
                        Zone pZone;
                        while (zoneElement) {
                            const char* zoneElementKey = zoneElement->GetName();
                            const char* zoneElementValue = zoneElement->GetString();
                            if (!Q_strcmp(zoneElementKey, "min_origin")) {
                                UTIL_StringToVector(pZone.minOrigin, zoneElementValue);
                            }
                            else if (!Q_strcmp(zoneElementKey, "max_origin")) {
                                UTIL_StringToVector(pZone.maxOrigin, zoneElementValue);
                            }
                            else if (!Q_strcmp(zoneElementKey, "parameter")) {
                                pZone.parameterName = zoneElementValue;
                            }
                            zoneElement = zoneElement->GetNextKey();
                        }
                        watcherZones->push_back(pZone);
                    }
                    else {
                        Warning("FMOD Adaptive Music - Found an unknown \"zones\" subkey in file\n");
                    }
                    zone = zone->GetNextKey();
                }
            }
            element = element->GetNextKey();
        }

        // Step 2: Init the watcher according to the gathered settings
        if (watcherType == nullptr) {
            Warning("FMOD Adaptive Music - Found a AdaptiveMusicWatcher to create but no type was provided\n");
            return;
        }
        if (!Q_strcmp(watcherType, "health")) {
            // Create and spawn the watcher entity, then set its params
            CBaseEntity* pNode = CreateEntityByName("adaptive_music_health_watcher");
            if (pNode) {
                DispatchSpawn(pNode);
                auto* healthWatcher = dynamic_cast<CAdaptiveMusicHealthWatcher *>(pNode);
                healthWatcher->SetAdaptiveMusicSystem(this);
                healthWatcher->SetParameterName(watcherParam);
                healthWatcher->Activate();
            }
            else {
                Warning("FMOD Adaptive Music - Failed to spawn a HealthWatcher entity\n");
            }
        }
        else if (!Q_strcmp(watcherType, "suit")) {
            // Create and spawn the watcher entity, then set its params
            CBaseEntity* pNode = CreateEntityByName("adaptive_music_suit_watcher");
            if (pNode) {
                DispatchSpawn(pNode);
                auto* suitWatcher = dynamic_cast<CAdaptiveMusicSuitWatcher *>(pNode);
                suitWatcher->SetAdaptiveMusicSystem(this);
                suitWatcher->SetParameterName(watcherParam);
                suitWatcher->Activate();
            }
            else {
                Warning("FMOD Adaptive Music - Failed to spawn a SuitWatcher entity\n");
            }
        }
        else if (!Q_strcmp(watcherType, "chased")) {
            // Create and spawn the watcher entity, then set its params
            CBaseEntity* pNode = CreateEntityByName("adaptive_music_chased_watcher");
            if (pNode) {
                DispatchSpawn(pNode);
                auto* chasedWatcher = dynamic_cast<CAdaptiveMusicChasedWatcher *>(pNode);
                chasedWatcher->SetAdaptiveMusicSystem(this);
                chasedWatcher->SetParameterName(watcherParam);
                chasedWatcher->Activate();
            }
            else {
                Warning("FMOD Adaptive Music - Failed to spawn a SuitWatcher entity\n");
            }
        }
        else if (!Q_strcmp(watcherType, "entity")) {
            // Create and spawn the watcher entity, then set its params
            CBaseEntity* pNode = CreateEntityByName("adaptive_music_entity_watcher");
            if (pNode) {
                DispatchSpawn(pNode);
                auto* entityWatcher = dynamic_cast<CAdaptiveMusicEntityWatcher *>(pNode);
                entityWatcher->SetAdaptiveMusicSystem(this);
                entityWatcher->SetEntityClass(watcherEntityClass);
                entityWatcher->SetEntityWatchedStatus(watcherEntityWatchedStatus);
                entityWatcher->SetParameterName(watcherParam);
                entityWatcher->Activate();
            }
            else {
                Warning("FMOD Adaptive Music - Failed to spawn a EntityWatcher entity\n");
            }
        }
        else if (!Q_strcmp(watcherType, "zone")) {
            // Step 3, for zone watchers: find the zones key and parse it to find the zones to set
            // Create and spawn the watcher entity, then set its params
            CBaseEntity* pNode = CreateEntityByName("adaptive_music_zone_watcher");
            if (pNode) {
                DispatchSpawn(pNode);
                auto* zoneWatcher = dynamic_cast<CAdaptiveMusicZoneWatcher *>(pNode);
                zoneWatcher->SetAdaptiveMusicSystem(this);
                zoneWatcher->SetZones(watcherZones);
                zoneWatcher->Activate();
            }
            else {
                Warning("FMOD Adaptive Music - Failed to spawn a ZoneWatcher entity\n");
            }
        }
        else {
            Warning("FMOD Adaptive Music - Unknown watcher type: %s\n", watcherType);
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
        auto* keyValue = new KeyValues("adaptive_music");
        if (keyValue->LoadFromFile(filesystem, szFullName, "MOD")) {
            Msg("FMOD Adaptive Music - Loading adaptive music data from '%s'\n", szFullName);
            KeyValues* keyValueSubset = keyValue->GetFirstSubKey();
            while (keyValueSubset) {
                ParseKeyValue(keyValueSubset);
                keyValueSubset = keyValueSubset->GetNextKey();
            }
        }
        else {
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
