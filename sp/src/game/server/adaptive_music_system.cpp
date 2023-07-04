//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The system for handling adaptive music
//
//=============================================================================//

#include "cbase.h"

#include "tier0/icommandline.h"
#include "igamesystem.h"
#include "filesystem.h"
#include <KeyValues.h>
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "soundenvelope.h"
#include "utldict.h"
#include "isaverestore.h"
#include "eventqueue.h"
#include "saverestore_utlvector.h"
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

//-----------------------------------------------------------------------------
// Purpose: Game system to kickstart the adaptive music
//-----------------------------------------------------------------------------
class CAdaptiveMusicSystem : public CAutoGameSystem {

    class CFMODEventListener : public IGameEventListener2 {
    public:
        CFMODEventListener() {
            gameeventmanager->AddListener(this, "player_spawn", true);
        }

        virtual void FireGameEvent(IGameEvent *pEvent) {
            if (Q_strcmp(pEvent->GetName(), "player_spawn") == 0) {
                g_AdaptiveMusicSystem.InitAdaptiveMusic();
            }
        }
    };

    CFMODEventListener *pFMODEventListener;
    CFMODEventListener &gFMODEventListener = *pFMODEventListener;

public:

    CAdaptiveMusicSystem() : CAutoGameSystem("CAdaptiveMusicSystem") {
    }

    virtual bool Init() {
        return true;
    }

    virtual void LevelInitPreEntity() {
        CalculateAdaptiveMusicState();
    }

    virtual void LevelInitPostEntity() {
        Msg("Start FMOD event listener");
        pFMODEventListener = new CFMODEventListener();
        gFMODEventListener = *pFMODEventListener;
        // Don't init right away but wait for the player to be spawned
        // InitAdaptiveMusic();
    }

    virtual void LevelShutdownPreEntity() {
        ShutDownAdaptiveMusic();
    }

    // Set the available ConVar if we can find adaptive music data for this level
    void CalculateAdaptiveMusicState() {
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

    void ParseKeyValue(KeyValues *keyValue) {
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
                    CSingleUserRecipientFilter filter(pBasePlayer); // set recipient
                    filter.MakeReliable();  // reliable transmission
                    UserMessageBegin(filter, "FMODLoadBank"); // create message
                    WRITE_STRING(elementValue);
                    MessageEnd(); //send message
                    /*
                    IGameEvent *pEvent = gameeventmanager->CreateEvent("fmod_loadbank");
                    if (pEvent) {
                        pEvent->SetString("bankname", elementValue);
                        gameeventmanager->FireEvent(pEvent);
                    }
                     */
                } else if (!Q_strcmp(elementKey, "event")) {
                    /*
                    IGameEvent *pEvent = gameeventmanager->CreateEvent("fmod_startevent");
                    if (pEvent) {
                        pEvent->SetString("eventpath", elementValue);
                        gameeventmanager->FireEvent(pEvent);
                    }
                     */
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

    void InitAdaptiveMusic() {
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

    void ShutDownAdaptiveMusic() {
        Msg("FMOD Adaptive Music - Shutting down adaptive music for the map\n");
    }

};

CAdaptiveMusicSystem g_AdaptiveMusicSystem;
