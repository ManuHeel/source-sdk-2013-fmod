//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//===========================================================================//

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

// FMOD Includes
#include "fmod_manager.h"

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

// useful helper func
#ifndef GAME_DLL

inline bool FStrEq(const char *sz1, const char *sz2) {
    return (Q_stricmp(sz1, sz2) == 0);
}

#endif

//---------------------------------------------------------------------------------
// Purpose: AdaptiveMusic Client Plugin
//---------------------------------------------------------------------------------
class CAdaptiveMusicClientPlugin : public IServerPluginCallbacks, public IGameEventListener {
public:
    CAdaptiveMusicClientPlugin();

    ~CAdaptiveMusicClientPlugin();

    //CFMODManager *pFMODManager;

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

private:
    int m_iClientCommandIndex;
};

//
// The plugin is a static singleton that is exported as an interface
//
CAdaptiveMusicClientPlugin g_AdaptiveMusicClientPlugin;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CAdaptiveMusicClientPlugin, IServerPluginCallbacks,
                                  INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_AdaptiveMusicClientPlugin);

//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CAdaptiveMusicClientPlugin::CAdaptiveMusicClientPlugin() {
    m_iClientCommandIndex = 0;
}

CAdaptiveMusicClientPlugin::~CAdaptiveMusicClientPlugin() {
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CAdaptiveMusicClientPlugin::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
    ConnectTier1Libraries(&interfaceFactory, 1);
    ConnectTier2Libraries(&interfaceFactory, 1);

    entityinfomanager = (IEntityInfoManager *) gameServerFactory(INTERFACEVERSION_ENTITYINFOMANAGER, NULL);
    if (!entityinfomanager) {
        Warning("Unable to load entityinfomanager, ignoring\n"); // this isn't fatal, we just won't be able to access entity data
    }

    playerinfomanager = (IPlayerInfoManager *) gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER, NULL);
    if (!playerinfomanager) {
        Warning("Unable to load playerinfomanager, ignoring\n"); // this isn't fatal, we just won't be able to access specific player data
    }

    gameinfomanager = (IGameInfoManager *) gameServerFactory(INTERFACEVERSION_GAMEINFOMANAGER, NULL);
    if (!gameinfomanager) {
        Warning("Unable to load gameinfomanager, ignoring\n");
    }

    engine = (IVEngineClient *) interfaceFactory(VENGINE_CLIENT_INTERFACE_VERSION, NULL);
    gameeventmanager = (IGameEventManager *) interfaceFactory(INTERFACEVERSION_GAMEEVENTSMANAGER, NULL);
    helpers = (IServerPluginHelpers *) interfaceFactory(INTERFACEVERSION_ISERVERPLUGINHELPERS, NULL);
    enginetrace = (IEngineTrace *) interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER, NULL);
    randomStr = (IUniformRandomStream *) interfaceFactory(VENGINE_SERVER_RANDOM_INTERFACE_VERSION, NULL);

    // get the interfaces we want to use
    if (!(engine && gameeventmanager && g_pFullFileSystem && helpers && enginetrace && randomStr)) {
        return false; // we require all these interface to function
    }

    if (playerinfomanager) {
        gpGlobals = playerinfomanager->GetGlobalVars();
    }

    MathLib_Init(2.2f, 2.2f, 0.0f, 2.0f);
    ConVar_Register(0);

    // Start the FMOD Engine
    //pFMODManager = FMODManager();
    //pFMODManager->SetEngineClient(engine);
    //pFMODManager->StartEngine();
    //FMODManager()->StartEngine();

    return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::Unload(void) {
    gameeventmanager->RemoveListener(this); // make sure we are unloaded from the event system

    ConVar_Unregister();
    DisconnectTier2Libraries();
    DisconnectTier1Libraries();

    // Stop the FMOD Engine
    //FMODManager()->StopEngine();

}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::Pause(void) {
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::UnPause(void) {
}

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char *CAdaptiveMusicClientPlugin::GetPluginDescription(void) {
    return "AdaptiveMusic Client Plugin, Manuel Russello";
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::LevelInit(char const *pMapName) {
    Msg("Level \"%s\" has been loaded\n", pMapName);
    gameeventmanager->AddListener(this, true);
}

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax) {
}

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::GameFrame(bool simulating) {
    if (simulating) {
    }
}

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::LevelShutdown(void) // !!!!this can get called multiple times per map change
{
    gameeventmanager->RemoveListener(this);
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::ClientActive(edict_t *pEntity) {
}

//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::ClientDisconnect(edict_t *pEntity) {
}

//---------------------------------------------------------------------------------
// Purpose: called on client connected and is put in the game
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::ClientPutInServer(edict_t *pEntity, char const *playername) {
    KeyValues *kv = new KeyValues("msg");
    kv->SetString("title", "AdaptiveMusic Client Plugin");
    kv->SetString("msg", "AdaptiveMusic Client Plugin text text text text text text text text");
    kv->SetColor("color", Color(255, 255, 255, 0));
    kv->SetInt("level", 5);
    kv->SetInt("time", 10);
    helpers->CreateMessage(pEntity, DIALOG_MSG, kv, this);
    kv->deleteThis();
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::SetCommandClient(int index) {
    m_iClientCommandIndex = index;
}

void ClientPrint(edict_t *pEdict, char *format, ...) {
    /*
    va_list argptr;
    static char string[1024];

    va_start(argptr, format);
    Q_vsnprintf(string, sizeof(string), format, argptr);
    va_end(argptr);

    engine->ClientPrintf(pEdict, string);
    */
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::ClientSettingsChanged(edict_t *pEdict) {
    /*
    if (playerinfomanager)
    {
        IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pEdict);

        const char *name = engine->GetClientConVarValue(engine->IndexOfEdict(pEdict), "name");

        // CAN'T use Q_stricmp here, this dll is made by 3rd parties and may not link to tier0/vstdlib
        if (playerinfo && name && playerinfo->GetName() &&
            stricmp(name, playerinfo->GetName())) // playerinfo may be NULL if the MOD doesn't support access to player data
                                                  // OR if you are accessing the player before they are fully connected
        {
            ClientPrint(pEdict, "Your name changed to \"%s\" (from \"%s\"\n", name, playerinfo->GetName());
            // this is the bad way to check this, the better option it to listen for the "player_changename" event in FireGameEvent()
            // this is here to give a real example of how to use the playerinfo interface
        }
    }
    */
}

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
PLUGIN_RESULT CAdaptiveMusicClientPlugin::ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName,
                                                        const char *pszAddress, char *reject, int maxrejectlen) {
    return PLUGIN_CONTINUE;
}

/*
CON_COMMAND( DoAskConnect, "Server plugin example of using the ask connect dialog" )
{
	if ( args.ArgC() < 2 )
	{
		Warning ( "DoAskConnect <server IP>\n" );
	}
	else
	{
		const char *pServerIP = args.Arg( 1 );

		KeyValues *kv = new KeyValues( "menu" );
		kv->SetString( "title", pServerIP );	// The IP address of the server to connect to goes in the "title" field.
		kv->SetInt( "time", 3 );

		for ( int i=1; i < gpGlobals->maxClients; i++ )
		{
			edict_t *pEdict = engine->PEntityOfEntIndex( i );
			if ( pEdict )
			{
				helpers->CreateMessage( pEdict, DIALOG_ASKCONNECT, kv, &g_AdaptiveMusicClientPlugin );
			}
		}

		kv->deleteThis();
	}
}
*/

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------

PLUGIN_RESULT CAdaptiveMusicClientPlugin::ClientCommand(edict_t *pEntity, const CCommand &args) {
    /*
    const char *pcmd = args[0];

    if ( !pEntity || pEntity->IsFree() )
    {
        return PLUGIN_CONTINUE;
    }

    if ( FStrEq( pcmd, "menu" ) )
    {
        KeyValues *kv = new KeyValues( "menu" );
        kv->SetString( "title", "You've got options, hit ESC" );
        kv->SetInt( "level", 1 );
        kv->SetColor( "color", Color( 255, 0, 0, 255 ));
        kv->SetInt( "time", 20 );
        kv->SetString( "msg", "Pick an option\nOr don't." );

        for( int i = 1; i < 9; i++ )
        {
            char num[10], msg[10], cmd[10];
            Q_snprintf( num, sizeof(num), "%i", i );
            Q_snprintf( msg, sizeof(msg), "Option %i", i );
            Q_snprintf( cmd, sizeof(cmd), "option%i", i );

            KeyValues *item1 = kv->FindKey( num, true );
            item1->SetString( "msg", msg );
            item1->SetString( "command", cmd );
        }

        helpers->CreateMessage( pEntity, DIALOG_MENU, kv, this );
        kv->deleteThis();
        return PLUGIN_STOP; // we handled this function
    }
    else if ( FStrEq( pcmd, "rich" ) )
    {
        KeyValues *kv = new KeyValues( "menu" );
        kv->SetString( "title", "A rich message" );
        kv->SetInt( "level", 1 );
        kv->SetInt( "time", 20 );
        kv->SetString( "msg", "This is a long long long text string.\n\nIt also has line breaks." );

        helpers->CreateMessage( pEntity, DIALOG_TEXT, kv, this );
        kv->deleteThis();
        return PLUGIN_STOP; // we handled this function
    }
    else if ( FStrEq( pcmd, "msg" ) )
    {
        KeyValues *kv = new KeyValues( "menu" );
        kv->SetString( "title", "Just a simple hello" );
        kv->SetInt( "level", 1 );
        kv->SetInt( "time", 20 );

        helpers->CreateMessage( pEntity, DIALOG_MSG, kv, this );
        kv->deleteThis();
        return PLUGIN_STOP; // we handled this function
    }
    else if ( FStrEq( pcmd, "entry" ) )
    {
        KeyValues *kv = new KeyValues( "entry" );
        kv->SetString( "title", "Stuff" );
        kv->SetString( "msg", "Enter something" );
        kv->SetString( "command", "say" ); // anything they enter into the dialog turns into a say command
        kv->SetInt( "level", 1 );
        kv->SetInt( "time", 20 );

        helpers->CreateMessage( pEntity, DIALOG_ENTRY, kv, this );
        kv->deleteThis();
        return PLUGIN_STOP; // we handled this function
    }
    */
    return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client is authenticated
//---------------------------------------------------------------------------------
PLUGIN_RESULT CAdaptiveMusicClientPlugin::NetworkIDValidated(const char *pszUserName, const char *pszNetworkID) {
    return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a cvar value query is finished
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t *pPlayerEntity,
                                                          EQueryCvarValueStatus eStatus, const char *pCvarName,
                                                          const char *pCvarValue) {
    // Msg( "Cvar query (cookie: %d, status: %d) - name: %s, value: %s\n", iCookie, eStatus, pCvarName, pCvarValue );
}

void CAdaptiveMusicClientPlugin::OnEdictAllocated(edict_t *edict) {
}

void CAdaptiveMusicClientPlugin::OnEdictFreed(const edict_t *edict) {
}

//---------------------------------------------------------------------------------
// Purpose: called when an event is fired
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::FireGameEvent(KeyValues *event) {
    const char *name = event->GetName();
    Msg("CAdaptiveMusicClientPlugin::FireGameEvent: Got event \"%s\"\n", name);
}

//---------------------------------------------------------------------------------
// Purpose: an example of how to implement a new command
//---------------------------------------------------------------------------------
CON_COMMAND(AM_CP_version, "Prints the version of the AdaptiveMusic Client Plugin") {
    Msg("AdaptiveMusic Client Plugin v0.0.1\n");
}
