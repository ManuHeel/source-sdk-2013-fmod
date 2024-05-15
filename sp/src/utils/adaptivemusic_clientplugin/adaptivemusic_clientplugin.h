#ifndef ADAPTIVEMUSIC_CLIENTPLUGIN_H
#define ADAPTIVEMUSIC_CLIENTPLUGIN_H
#ifdef _WIN32
#pragma once
#endif

#include <stdio.h>

// #define GAME_DLL
#ifdef GAME_DLL
#include "cbase.h"
#endif

#include <stdio.h>
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#include "cdll_int.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "Color.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"
#include "tier2/tier2.h"
#include "game/server/pluginvariant.h"
#include "game/server/iplayerinfo.h"
#include "game/server/ientityinfo.h"
#include "game/server/igameinfo.h"
#include "usermessages.h"

// FMOD Includes
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include "fmod_errors.h"

// Custom Includes
#include "adaptivemusic_watcher.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Interfaces from the engine
IVEngineClient *engine = NULL;                 // helper functions (messaging clients, loading content, making entities, running commands, etc)
IGameEventManager *gameeventmanager_ = NULL; // game events interface
#ifndef GAME_DLL
#define gameeventmanager gameeventmanager_
#endif
IPlayerInfoManager *playerinfomanager = NULL; // game dll interface to interact with players
IEntityInfoManager *entityinfomanager = NULL; // game dll interface to interact with all entities (like IPlayerInfo)
IGameInfoManager *gameinfomanager = NULL;      // game dll interface to get data from game rules directly
IServerPluginHelpers *helpers = NULL;          // special 3rd party plugin helpers from the engine
IUniformRandomStream *randomStr = NULL;
IEngineTrace *enginetrace = NULL;

CGlobalVars *gpGlobals = NULL;

// FMOD client specific variables
FMOD::Studio::System *fmodStudioSystem;
FMOD::Studio::Bank *loadedFmodStudioBank;
char *loadedFmodStudioBankName;
FMOD::Studio::Bank *loadedFmodStudioStringsBank;
FMOD::Studio::EventDescription *loadedFmodStudioEventDescription;
char *loadedFmodStudioEventPath;
FMOD::Studio::EventInstance *createdFmodStudioEventInstance;

// Adaptive Music System (inherited from the server-side)
ConVar adaptive_music_available("adaptive_music_available", "0", FCVAR_NONE,
                                "Automatically set by the game when an adaptive music file is available for the current map.");
const char *loadedBankName;
const char *startedEventPath;

//---------------------------------------------------------------------------------
// Purpose: AdaptiveMusic Client Plugin
//---------------------------------------------------------------------------------
class CAdaptiveMusicClientPlugin : public IServerPluginCallbacks, public IGameEventListener {
public:
    CAdaptiveMusicClientPlugin();

    ~CAdaptiveMusicClientPlugin();

    //CAdaptiveMusicClientPlugin *pFMODManager;

    // IServerPluginCallbacks methods
    virtual bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory);

    virtual void Unload(void);

    virtual void Pause(void);

    virtual void UnPause(void);

    virtual const char *GetPluginDescription(void);

    virtual void LevelInit(char const *pMapName);

    virtual void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);

    virtual void GameFrame(bool simulating);

    virtual void LevelShutdown(void);

    virtual void ClientActive(edict_t *pEntity);

    virtual void ClientDisconnect(edict_t *pEntity);

    virtual void ClientPutInServer(edict_t *pEntity, char const *playername);

    virtual void SetCommandClient(int index);

    virtual void ClientSettingsChanged(edict_t *pEdict);

    virtual PLUGIN_RESULT
    ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject,
                  int maxrejectlen);

    virtual PLUGIN_RESULT ClientCommand(edict_t *pEntity, const CCommand &args);

    virtual PLUGIN_RESULT NetworkIDValidated(const char *pszUserName, const char *pszNetworkID);

    virtual void
    OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus,
                             const char *pCvarName, const char *pCvarValue);

    virtual void OnEdictAllocated(edict_t *edict);

    virtual void OnEdictFreed(const edict_t *edict);

    // IGameEventListener Interface
    virtual void FireGameEvent(KeyValues *event);

    virtual int GetCommandIndex() { return m_iClientCommandIndex; }

    // FMOD specific methods
    virtual int StartEngine();

    virtual int StopEngine();

    virtual int LoadBank(const char *bankName);

    virtual int StartEvent(const char *eventPath);

    virtual int StopEvent(const char *eventPath);

    virtual int SetGlobalParameter(const char *parameterName, float value);

    virtual int SetPausedState(bool pausedState);

    virtual const char *GetBankPath(const char *bankName);

    // Adaptive Music System (inherited from the server-side)
    void CalculateAdaptiveMusicState();

    void InitAdaptiveMusic();

    void ShutDownAdaptiveMusic();

    void ParseKeyValue(KeyValues *keyValue);

private:
    int m_iClientCommandIndex;
};

//
// The plugin is a static singleton that is exported as an interface
//
CAdaptiveMusicClientPlugin g_AdaptiveMusicClientPlugin;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CAdaptiveMusicClientPlugin, IServerPluginCallbacks,
                                  INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_AdaptiveMusicClientPlugin);

#endif