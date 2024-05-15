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
#include "usermessages.h"

// FMOD Includes
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include "fmod_errors.h"

// Custom Includes
#include "adaptivemusic_clientplugin.h"
#include "adaptivemusic_watcher.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// useful helper func
#ifndef GAME_DLL

inline bool FStrEq(const char *sz1, const char *sz2) {
    return (Q_stricmp(sz1, sz2) == 0);
}

#endif

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
    StartEngine();

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
    StopEngine();

}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::Pause(void) {
    Msg("Pause\n");
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::UnPause(void) {
    Msg("Unpause\n");
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
    gameeventmanager->AddListener(this, true);
    CalculateAdaptiveMusicState();
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
        // TODO : Use these as pause/unpause source
    } else {
        // TODO : Use these as pause/unpause source
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
    InitAdaptiveMusic();
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
    /*
    KeyValues *kv = new KeyValues("msg");
    kv->SetString("title", "AdaptiveMusic Client Plugin");
    kv->SetString("msg", "AdaptiveMusic Client Plugin text text text text text text text text");
    kv->SetColor("color", Color(255, 255, 255, 0));
    kv->SetInt("level", 5);
    kv->SetInt("time", 10);
    helpers->CreateMessage(pEntity, DIALOG_MSG, kv, this);
    kv->deleteThis();
     */
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

//-----------------------------------------------------------------------------
// FMOD "client" specific methods
//-----------------------------------------------------------------------------

//// HELPER FUNCTIONS
//-----------------------------------------------------------------------------
// Purpose: Concatenate 2 strings together
// Input:
// - str1: The starting string
// - str2: The ending string
// Output: The joined 2 strings
//-----------------------------------------------------------------------------
const char *Concatenate(const char *str1, const char *str2) {
    size_t len1 = 0;
    size_t len2 = 0;
    while (str1[len1] != '\0')
        ++len1;
    while (str2[len2] != '\0')
        ++len2;
    char *result = new char[len1 + len2 + 1]; // +1 for the null terminator
    for (size_t i = 0; i < len1; ++i)
        result[i] = str1[i];
    for (size_t i = 0; i < len2; ++i)
        result[len1 + i] = str2[i];
    result[len1 + len2] = '\0';
    return result;
}

//// END HELPER FUNCTIONS

//-----------------------------------------------------------------------------
// Purpose: Provide a console command to print the FMOD Engine Status
//-----------------------------------------------------------------------------
void CC_GetStatus() {
    bool isValid = fmodStudioSystem->isValid();
    if (isValid) {
        Msg("FMOD Client - Engine is currently running\n");
    } else {
        Msg("FMOD Client - Engine is not running\n");
    }
}

ConCommand getStatus("fmod_getstatus", CC_GetStatus, "FMOD: Get current status of the FMOD Manager");

//-----------------------------------------------------------------------------
// Purpose: Load an FMOD Bank
// Input: The name of the FMOD Bank to load
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicClientPlugin::LoadBank(const char *bankName) {
    if (loadedFmodStudioBankName != nullptr && (strcmp(bankName, loadedFmodStudioBankName) == 0)) {
        // Bank is already loaded
        Log("FMOD Client - Bank requested for loading but already loaded (%s)\n", bankName);
    } else {
        // Load the requested bank
        const char *bankPath = CAdaptiveMusicClientPlugin::GetBankPath(bankName);
        FMOD_RESULT result;
        result = fmodStudioSystem->loadBankFile(bankPath, FMOD_STUDIO_LOAD_BANK_NORMAL,
                                                &loadedFmodStudioBank);
        if (result != FMOD_OK) {
            Warning("FMOD Client - Could not load Bank (%s). Error: (%d) %s\n", bankName, result,
                    FMOD_ErrorString(result));
            return (-1);
        }
        const char *bankStringsName = Concatenate(bankName, ".strings");
        result = fmodStudioSystem->loadBankFile(CAdaptiveMusicClientPlugin::GetBankPath(bankStringsName),
                                                FMOD_STUDIO_LOAD_BANK_NORMAL,
                                                &loadedFmodStudioStringsBank);
        if (result != FMOD_OK) {
            Warning("FMOD Client - Could not load Strings Bank (%s). Error: (%d) %s\n", bankStringsName, result,
                    FMOD_ErrorString(result));
            return (-1);
        }
        Log("FMOD Client - Bank successfully loaded (%s)\n", bankName);
        delete[] loadedFmodStudioBankName;
        loadedFmodStudioBankName = new char[strlen(bankName) + 1];
        strcpy(loadedFmodStudioBankName, bankName);
        return (0);
    }

    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Provide a console command to load a FMOD Bank
// Input: The name of the FMOD Bank to load as ConCommand argument
//-----------------------------------------------------------------------------
void CC_LoadBank(const CCommand &args) {
    if (args.ArgC() < 1 || strcmp(args.Arg(1), "") == 0) {
        Msg("Usage: fmod_loadbank <bankname>\n");
        return;
    }
    g_AdaptiveMusicClientPlugin.LoadBank(args.Arg(1));
}

ConCommand loadBank("fmod_loadbank", CC_LoadBank, "FMOD: Load a bank");

//-----------------------------------------------------------------------------
// Purpose: Provide a UserMessage handler to load a FMOD Bank
// Input: The name of the FMOD Bank to load
//-----------------------------------------------------------------------------
void MsgFunc_LoadBank(bf_read &msg) {
    char szString[256];
    msg.ReadString(szString, sizeof(szString));
    g_AdaptiveMusicClientPlugin.LoadBank(szString);
}

//-----------------------------------------------------------------------------
// Purpose: Start an FMOD Event
// Input: The name of the FMOD Event to start
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicClientPlugin::StartEvent(const char *eventPath) {
    if (loadedFmodStudioEventPath != nullptr && (Q_strcmp(eventPath, loadedFmodStudioEventPath) == 0)) {
        // Event is already loaded
        Log("FMOD Client - Event requested for loading but already loaded (%s)\n", eventPath);
    } else {
        // Event is new
        if (loadedFmodStudioEventPath != nullptr && (Q_strcmp(loadedFmodStudioEventPath, "") != 0)) {
            // Stop the currently playing event
            StopEvent(loadedFmodStudioEventPath);
        }
        const char *fullEventPath = Concatenate("event:/", eventPath);
        FMOD_RESULT result;
        result = fmodStudioSystem->getEvent(fullEventPath, &loadedFmodStudioEventDescription);
        result = loadedFmodStudioEventDescription->createInstance(&createdFmodStudioEventInstance);
        result = createdFmodStudioEventInstance->start();
        fmodStudioSystem->update();
        if (result != FMOD_OK) {
            Warning("FMOD Client - Could not start Event (%s). Error: (%d) %s\n", eventPath, result,
                    FMOD_ErrorString(result));
            return (-1);
        }
        Log("FMOD Client - Event successfully started (%s)\n", eventPath);
        delete[] loadedFmodStudioEventPath;
        loadedFmodStudioEventPath = new char[strlen(eventPath) + 1];
        strcpy(loadedFmodStudioEventPath, eventPath);
    }
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Provide a console command to start an FMOD Event
// Input: The name of the FMOD Event to start as ConCommand argument
//-----------------------------------------------------------------------------
void CC_StartEvent(const CCommand &args) {
    if (args.ArgC() < 1 || Q_strcmp(args.Arg(1), "") == 0) {
        Msg("Usage: fmod_startevent <eventpath>\n");
        return;
    }
    g_AdaptiveMusicClientPlugin.StartEvent(args.Arg(1));
}

ConCommand startEvent("fmod_startevent", CC_StartEvent, "FMOD: Start an event");

//-----------------------------------------------------------------------------
// Purpose: Provide a UserMessage handler to start an FMOD Event
// Input: The name of the FMOD Event to start
//-----------------------------------------------------------------------------
void MsgFunc_StartEvent(bf_read &msg) {
    char szString[256];
    msg.ReadString(szString, sizeof(szString));
    g_AdaptiveMusicClientPlugin.StartEvent(szString);
}

//-----------------------------------------------------------------------------
// Purpose: Stop an FMOD Event
// Input: The name of the FMOD Event to stop
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicClientPlugin::StopEvent(const char *eventPath) {
    const char *fullEventPath = Concatenate("event:/", eventPath);
    FMOD_RESULT result;
    result = fmodStudioSystem->getEvent(fullEventPath, &loadedFmodStudioEventDescription);
    result = loadedFmodStudioEventDescription->releaseAllInstances();
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        Warning("FMOD Client - Could not stop Event (%s). Error: (%d) %s\n", eventPath, result,
                FMOD_ErrorString(result));
        return (-1);
    }
    Log("FMOD Client - Event successfully stopped (%s)\n", eventPath);
    delete[] loadedFmodStudioEventPath;
    loadedFmodStudioEventPath = new char[strlen("") + 1];
    strcpy(loadedFmodStudioEventPath, "");
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Provide a console command to stop an FMOD Event
// Input: The name of the FMOD Event to load as ConCommand argument
//-----------------------------------------------------------------------------
void CC_StopEvent(const CCommand &args) {
    if (args.ArgC() < 1 || Q_strcmp(args.Arg(1), "") == 0) {
        Msg("Usage: fmod_stopevent <eventpath>\n");
        return;
    }
    g_AdaptiveMusicClientPlugin.StopEvent(args.Arg(1));
}

ConCommand stopEvent("fmod_stopevent", CC_StopEvent, "FMOD: Stop an event");

//-----------------------------------------------------------------------------
// Purpose: Provide a UserMessage handler to stop an FMOD Event
// Input: The name of the FMOD Event to stop
//-----------------------------------------------------------------------------
void MsgFunc_StopEvent(bf_read &msg) {
    char szString[256];
    msg.ReadString(szString, sizeof(szString));
    g_AdaptiveMusicClientPlugin.StopEvent(szString);
}

//-----------------------------------------------------------------------------
// Purpose: Set the value for a global FMOD Parameter
// Input:
// - parameterName: The name of the FMOD Parameter to set
// - value: The value to set the FMOD Parameter to
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicClientPlugin::SetGlobalParameter(const char *parameterName, float value) {
    ;
    FMOD_RESULT result;
    result = fmodStudioSystem->setParameterByName(parameterName, value);
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        Warning("FMOD Client - Could not set Global Parameter value (%s) (%f). Error: (%d) %s\n", parameterName, value,
                result, FMOD_ErrorString(result));
        return (-1);
    }
    Log("FMOD Client - Global Parameter %s set to %f\n", parameterName, value);
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Provide a console command to set the value for a global FMOD Parameter
// Input:
// - Arg(1): The name of the FMOD Parameter to set (to load as ConCommand argument)
// - Arg(2): The value to set the FMOD Parameter to (to load as ConCommand argument)
//-----------------------------------------------------------------------------
void CC_SetGlobalParameter(const CCommand &args) {
    if (args.ArgC() < 2 || Q_strcmp(args.Arg(1), "") == 0 || Q_strcmp(args.Arg(2), "") == 0) {
        Msg("Usage: fmod_setglobalparameter <parametername> <value>\n");
        return;
    }
    g_AdaptiveMusicClientPlugin.SetGlobalParameter(args.Arg(1), atof(args.Arg(2)));
}

ConCommand setGlobalParameter("fmod_setglobalparameter", CC_SetGlobalParameter, "FMOD: Set a global parameter");

//-----------------------------------------------------------------------------
// Purpose: Provide a UserMessage handler to set the value for a global FMOD Parameter
// Input:
// - The name of the FMOD Parameter to set (to load as ConCommand argument)
// - The value to set the FMOD Parameter to (to load as ConCommand argument)
//-----------------------------------------------------------------------------
void MsgFunc_SetGlobalParameter(bf_read &msg) {
    char szString[256];
    msg.ReadString(szString, sizeof(szString));
    float szFloat;
    szFloat = msg.ReadFloat();
    g_AdaptiveMusicClientPlugin.SetGlobalParameter(szString, szFloat);
}

//-----------------------------------------------------------------------------
// Purpose: Start the FMOD Studio System and initialize it
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicClientPlugin::StartEngine() {
    // Startup FMOD Studio System
    Msg("FMOD Client - Starting Engine\n");
    FMOD_RESULT result;
    result = FMOD::Studio::System::create(&fmodStudioSystem);
    if (result != FMOD_OK) {
        Error("FMOD Client - Engine error! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    result = fmodStudioSystem->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK) {
        Error("FMOD Client - Engine error! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    Log("FMOD Client - Engine successfully started\n");

    // Handle the usermessages (registered in hl2_usermessages.cpp)
    Log("FMOD Client - Hooking up the UserMessages\n");
    //usermessages->HookMessage("FMODLoadBank", MsgFunc_LoadBank);
    //usermessages->HookMessage("FMODStartEvent", MsgFunc_StartEvent);
    //usermessages->HookMessage("FMODStopEvent", MsgFunc_StopEvent);
    //usermessages->HookMessage("FMODSetGlobalParameter", MsgFunc_SetGlobalParameter);
    Log("FMOD Client - Successfully hooked up the UserMessages\n");

    // Start the infinitely-looping Client-Side watcher
    //StartClientSideWatcher();
    // TODO: Replace this with Pause/Unpause interface methods

    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Stop the FMOD Studio System
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CAdaptiveMusicClientPlugin::StopEngine() {
    Msg("FMOD Client - Stopping Engine\n");
    FMOD_RESULT result;
    result = fmodStudioSystem->release();
    if (result != FMOD_OK) {
        Error("FMOD Client - Engine error! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    Log("FMOD Client - Engine successfully stopped\n");

    // Stop the infinitely-looping Client-Side watcher
    // StopClientSideWatcher();
    // TODO: Replace this with Pause/Unpause interface methods

    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: TODO
// Output: TODO
//-----------------------------------------------------------------------------
int CAdaptiveMusicClientPlugin::SetPausedState(bool pausedState) {
    Log("FMOD Client - Setting master bus paused state to %d\n", pausedState);
    FMOD::Studio::Bus *bus;
    FMOD_RESULT result;
    result = fmodStudioSystem->getBus("bus:/", &bus);
    if (result != FMOD_OK) {
        Msg("FMOD Client - Could not find the master bus! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    result = bus->setPaused(pausedState);
    fmodStudioSystem->update();
    if (result != FMOD_OK) {
        Msg("FMOD Client - Could not pause the master bus! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }

    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Sanitize the name of an FMOD Bank, adding ".bank" if it's not already present
// Input: The FMOD Bank name to sanitize
// Output: The sanitized Bank name (same as the initial if it was already ending with ".bank")
//-----------------------------------------------------------------------------
const char *SanitizeBankName(const char *bankName) {
    const char *bankExtension = ".bank";
    size_t bankNameLength = strlen(bankName);
    size_t bankExtensionLength = strlen(bankExtension);
    if (bankNameLength >= bankExtensionLength &&
        Q_strcmp(bankName + bankNameLength - bankExtensionLength, bankExtension)
        == 0) {
        return bankName;
    }
    return Concatenate(bankName, bankExtension);
}

//-----------------------------------------------------------------------------
// Purpose: Get the path of a Bank file in the /sound/fmod/banks folder
// Input: The FMOD Bank name to locate
// Output: The FMOD Bank's full path from the file system
//-----------------------------------------------------------------------------
const char *CAdaptiveMusicClientPlugin::GetBankPath(const char *bankName) {
    const char *sanitizedBankName = SanitizeBankName(bankName);
    char *bankPath = new char[512];
    Q_snprintf(bankPath, 512, "%s/sound/fmod/banks/%s", engine->GetGameDirectory(), sanitizedBankName);
    // convert backwards slashes to forward slashes
    for (int i = 0; i < 512; i++) {
        if (bankPath[i] == '\\')
            bankPath[i] = '/';
    }
    return bankPath;
}

//-----------------------------------------------------------------------------
// Adaptive Music System (inherited from the Server-side)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Checks if the level has adaptive music data
// Sets the available ConVar if we can find adaptive music data for this level
//-----------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::CalculateAdaptiveMusicState() {
    char szFullName[512];
    Q_snprintf(szFullName, sizeof(szFullName), "maps/%s_adaptivemusic.txt", STRING(gpGlobals->mapname));
    if (g_pFullFileSystem->FileExists(szFullName)) {
        Msg("FMOD Adaptive Music - Found adaptive music file, '%s'\n", szFullName);
        adaptive_music_available.SetValue(true);
    } else {
        Msg("FMOD Adaptive Music - Could not find adaptive music file, '%s'\n", szFullName);
        adaptive_music_available.SetValue(false);
        //ShutDownAdaptiveMusic();
    }
}

//-----------------------------------------------------------------------------
// Purpose: Start the AdaptiveMusic up
// Finds the adaptive music file, parse it and initialize everything related to AdaptiveMusic for this level
//-----------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::InitAdaptiveMusic() {
    // If we know there's Adaptive Music data, initialize the Adaptive Music
    if (adaptive_music_available.GetBool()) {
        Msg("FMOD Adaptive Music - Initializing the map's adaptive music data\n");
        // Find the adaptive music file
        char szFullName[512];
        Q_snprintf(szFullName, sizeof(szFullName), "maps/%s_adaptivemusic.txt", STRING(gpGlobals->mapname));
        auto *keyValue = new KeyValues("adaptive_music");
        if (keyValue->LoadFromFile(g_pFullFileSystem, szFullName, "MOD")) {
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
void CAdaptiveMusicClientPlugin::ShutDownAdaptiveMusic() {
    Msg("FMOD Adaptive Music - Shutting down adaptive music for the map\n");
    if (startedEventPath != nullptr) {
        StopEvent(startedEventPath);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Parses the provided KeyValue containing adaptive music data from a file
// Input: A KeyValue object, subset of an adaptive music file
//-----------------------------------------------------------------------------
void CAdaptiveMusicClientPlugin::ParseKeyValue(KeyValues *keyValue) {
    const char *keyValueName = keyValue->GetName();
    Log("FMOD Adaptive Music - Parsing the KeyValue '%s'\n", keyValueName);
    if (!Q_strcmp(keyValueName, "globals")) {
        // The key-value element is defining the global parameters of the map
        KeyValues *element = keyValue->GetFirstSubKey();
        while (element) {
            const char *elementKey = element->GetName();
            const char *elementValue = element->GetString();
            Log("FMOD Adaptive Music - %s: %s\n", elementKey, elementValue);
            if (!Q_strcmp(elementKey, "bank")) {
                loadedBankName = elementValue;
                Log("FMOD Adaptive Music - Telling the FMOD Client to load the bank '%s'\n", loadedBankName);
                // Load the FMOD Bank
                LoadBank(elementValue);
            } else if (!Q_strcmp(elementKey, "event")) {
                startedEventPath = elementValue;
                Log("FMOD Adaptive Music - Telling the FMOD Client to start the event '%s'\n", startedEventPath);
                // Start the Event
                StartEvent(elementValue);
            }
            element = element->GetNextKey();
        }
    }
    /*
    else if (!Q_strcmp(keyValueName, "watcher")) {
        // The key-value element is defining a watcher and its params for the map
        KeyValues* element = keyValue->GetFirstSubKey();

        // Watcher settings
        const char* watcherType = nullptr;
        const char* watcherParam = nullptr;
        const char* watcherEntityClass = nullptr;
        const char* watcherEntityWatchedStatus = nullptr;
        const char* watcherEntityWatchedScene = nullptr;
        auto* watcherZones = new std::list<Zone>();
        auto* watcherScenes = new std::list<Scene>();

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
            else if (!Q_strcmp(elementKey, "entity_watched_scene")) {
                watcherEntityWatchedScene = elementValue;
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
            else if (!Q_strcmp(elementKey, "scenes")) {
                // Scene list settings
                KeyValues* scene = element->GetFirstSubKey();
                while (scene) {
                    const char* sceneKey = scene->GetName();
                    if (!Q_strcmp(sceneKey, "scene")) {
                        KeyValues* sceneElement = scene->GetFirstSubKey();
                        // Create a new Scene struct
                        Scene pScene;
                        while (sceneElement) {
                            const char* sceneElementKey = sceneElement->GetName();
                            const char* sceneElementValue = sceneElement->GetString();
                            if (!Q_strcmp(sceneElementKey, "scene_name")) {
                                pScene.sceneName = sceneElementValue;
                            }
                            else if (!Q_strcmp(sceneElementKey, "scene_state")) {
                                pScene.stateName = sceneElementValue;
                            }
                            else if (!Q_strcmp(sceneElementKey, "parameter")) {
                                pScene.parameterName = sceneElementValue;
                            }
                            sceneElement = sceneElement->GetNextKey();
                        }
                        watcherScenes->push_back(pScene);
                    }
                    else {
                        Warning("FMOD Adaptive Music - Found an unknown \"scenes\" subkey in file\n");
                    }
                    scene = scene->GetNextKey();
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
                entityWatcher->SetEntityWatchedScene(watcherEntityWatchedScene);
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
        else if (!Q_strcmp(watcherType, "scene")) {
            // Step 3, for scene watchers: find the scenes key and parse it to find the scenes to set
            // Create and spawn the watcher entity, then set its params
            CBaseEntity* pNode = CreateEntityByName("adaptive_music_scene_watcher");
            if (pNode) {
                DispatchSpawn(pNode);
                auto* sceneWatcher = dynamic_cast<CAdaptiveMusicSceneWatcher *>(pNode);
                sceneWatcher->SetAdaptiveMusicSystem(this);
                sceneWatcher->SetScenes(watcherScenes);
                sceneWatcher->Activate();
            }
            else {
                Warning("FMOD Adaptive Music - Failed to spawn a ZoneWatcher entity\n");
            }
        }
        else {
            Warning("FMOD Adaptive Music - Unknown watcher type: %s\n", watcherType);
        }
    }
     */
}