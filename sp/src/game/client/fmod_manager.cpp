#include "cbase.h"
#include "fmod_manager.h"
#include "fmod_studio.hpp"
#include "fmod_errors.h"
#include "convar.h"
#include "usermessages.h"

using namespace FMOD;

CFMODManager gFMODManager;

Studio::System* fmodStudioSystem;
Studio::Bank* loadedFmodStudioBank;
char* loadedFmodStudioBankName;
Studio::Bank* loadedFmodStudioStringsBank;
Studio::EventDescription* loadedFmodStudioEventDescription;
char* loadedFmodStudioEventPath;
Studio::EventInstance* createdFmodStudioEventInstance;

CFMODManager* FMODManager() {
    return &gFMODManager;
}

CFMODManager::CFMODManager()
= default;

CFMODManager::~CFMODManager()
= default;

//// HELPER FUNCTIONS
//-----------------------------------------------------------------------------
// Purpose: Concatenate 2 strings together
// Input:
// - str1: The starting string
// - str2: The ending string
// Output: The joined 2 strings
//-----------------------------------------------------------------------------
const char* Concatenate(const char* str1, const char* str2) {
    size_t len1 = 0;
    size_t len2 = 0;
    while (str1[len1] != '\0')
        ++len1;
    while (str2[len2] != '\0')
        ++len2;
    char* result = new char[len1 + len2 + 1]; // +1 for the null terminator
    for (size_t i = 0; i < len1; ++i)
        result[i] = str1[i];
    for (size_t i = 0; i < len2; ++i)
        result[len1 + i] = str2[i];
    result[len1 + len2] = '\0';
    return result;
}

//// END HELPER FUNCTIONS
// EventListener
class CFMODEventListener : public IGameEventListener2 {
private:
    CFMODManager* pFMODManager;

public:
    CFMODEventListener(CFMODManager* pFMODManagerRef) {
        pFMODManager = pFMODManagerRef;
        gameeventmanager->AddListener(this, "server_shutdown", true);
        gameeventmanager->AddListener(this, "game_newmap", false);
    }

    //-----------------------------------------------------------------------------
    // Purpose: Triggered when a listened GameEvent is fired
    // Input: The fired GameEvent
    //-----------------------------------------------------------------------------
    virtual void FireGameEvent(IGameEvent* pEvent) {
        if (Q_strcmp(pEvent->GetName(), "game_newmap") == 0) {
            Msg("FMOD Manager - Client in a new map\n");
            //CFMODManager::StopEvent(loadedFmodStudioEventPath);
        }
    }
};

CFMODEventListener* pFMODEventListener;

//-----------------------------------------------------------------------------
// Purpose: Provide a console command to print the FMOD Engine Status
//-----------------------------------------------------------------------------
void CC_GetStatus() {
    bool isValid = fmodStudioSystem->isValid();
    if (isValid) {
        Msg("FMOD Client - Engine is currently running\n");
    }
    else {
        Msg("FMOD Client - Engine is not running\n");
    }
}

static ConCommand getStatus("fmod_getstatus", CC_GetStatus, "FMOD: Get current status of the FMOD Manager");

//-----------------------------------------------------------------------------
// Purpose: Load an FMOD Bank
// Input: The name of the FMOD Bank to load
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CFMODManager::LoadBank(const char* bankName) {
    if (loadedFmodStudioBankName != nullptr && (strcmp(bankName, loadedFmodStudioBankName) == 0)) {
        // Bank is already loaded
        Log("FMOD Client - Bank requested for loading but already loaded (%s)\n", bankName);
    }
    else {
        // Load the requested bank
        const char* bankPath = CFMODManager::GetBankPath(bankName);
        FMOD_RESULT result;
        result = fmodStudioSystem->loadBankFile(bankPath, FMOD_STUDIO_LOAD_BANK_NORMAL,
                                                &loadedFmodStudioBank);
        if (result != FMOD_OK) {
            Warning("FMOD Client - Could not load Bank (%s). Error: (%d) %s\n", bankName, result,
                    FMOD_ErrorString(result));
            return (-1);
        }
        const char* bankStringsName = Concatenate(bankName, ".strings");
        result = fmodStudioSystem->loadBankFile(CFMODManager::GetBankPath(bankStringsName),
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
void CC_LoadBank(const CCommand&args) {
    if (args.ArgC() < 1 || strcmp(args.Arg(1), "") == 0) {
        Msg("Usage: fmod_loadbank <bankname>\n");
        return;
    }
    CFMODManager::LoadBank(args.Arg(1));
}

static ConCommand loadBank("fmod_loadbank", CC_LoadBank, "FMOD: Load a bank");

//-----------------------------------------------------------------------------
// Purpose: Provide a UserMessage handler to load a FMOD Bank
// Input: The name of the FMOD Bank to load
//-----------------------------------------------------------------------------
void MsgFunc_LoadBank(bf_read&msg) {
    char szString[256];
    msg.ReadString(szString, sizeof(szString));
    CFMODManager::LoadBank(szString);
}

//-----------------------------------------------------------------------------
// Purpose: Start an FMOD Event
// Input: The name of the FMOD Event to start
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CFMODManager::StartEvent(const char* eventPath) {
    if (loadedFmodStudioEventPath != nullptr && (Q_strcmp(eventPath, loadedFmodStudioEventPath) == 0)) {
        // Event is already loaded
        Log("FMOD Client - Event requested for loading but already loaded (%s)\n", eventPath);
    }
    else {
        // Event is new
        if (loadedFmodStudioEventPath != nullptr && (Q_strcmp(loadedFmodStudioEventPath, "") != 0)) {
            // Stop the currently playing event
            StopEvent(loadedFmodStudioEventPath);
        }
        const char* fullEventPath = Concatenate("event:/", eventPath);
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
void CC_StartEvent(const CCommand&args) {
    if (args.ArgC() < 1 || Q_strcmp(args.Arg(1), "") == 0) {
        Msg("Usage: fmod_startevent <eventpath>\n");
        return;
    }
    CFMODManager::StartEvent(args.Arg(1));
}

static ConCommand startEvent("fmod_startevent", CC_StartEvent, "FMOD: Start an event");

//-----------------------------------------------------------------------------
// Purpose: Provide a UserMessage handler to start an FMOD Event
// Input: The name of the FMOD Event to start
//-----------------------------------------------------------------------------
void MsgFunc_StartEvent(bf_read&msg) {
    char szString[256];
    msg.ReadString(szString, sizeof(szString));
    CFMODManager::StartEvent(szString);
}

//-----------------------------------------------------------------------------
// Purpose: Stop an FMOD Event
// Input: The name of the FMOD Event to stop
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CFMODManager::StopEvent(const char* eventPath) {
    const char* fullEventPath = Concatenate("event:/", eventPath);
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
void CC_StopEvent(const CCommand&args) {
    if (args.ArgC() < 1 || Q_strcmp(args.Arg(1), "") == 0) {
        Msg("Usage: fmod_stopevent <eventpath>\n");
        return;
    }
    CFMODManager::StopEvent(args.Arg(1));
}

static ConCommand stopEvent("fmod_stopevent", CC_StopEvent, "FMOD: Stop an event");

//-----------------------------------------------------------------------------
// Purpose: Provide a UserMessage handler to stop an FMOD Event
// Input: The name of the FMOD Event to stop
//-----------------------------------------------------------------------------
void MsgFunc_StopEvent(bf_read&msg) {
    char szString[256];
    msg.ReadString(szString, sizeof(szString));
    CFMODManager::StopEvent(szString);
}

//-----------------------------------------------------------------------------
// Purpose: Set the value for a global FMOD Parameter
// Input:
// - parameterName: The name of the FMOD Parameter to set
// - value: The value to set the FMOD Parameter to
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CFMODManager::SetGlobalParameter(const char* parameterName, float value) {
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
void CC_SetGlobalParameter(const CCommand&args) {
    if (args.ArgC() < 2 || Q_strcmp(args.Arg(1), "") == 0 || Q_strcmp(args.Arg(2), "") == 0) {
        Msg("Usage: fmod_setglobalparameter <parametername> <value>\n");
        return;
    }
    CFMODManager::SetGlobalParameter(args.Arg(1), atof(args.Arg(2)));
}

static ConCommand setGlobalParameter("fmod_setglobalparameter", CC_SetGlobalParameter, "FMOD: Set a global parameter");

//-----------------------------------------------------------------------------
// Purpose: Provide a UserMessage handler to set the value for a global FMOD Parameter
// Input:
// - The name of the FMOD Parameter to set (to load as ConCommand argument)
// - The value to set the FMOD Parameter to (to load as ConCommand argument)
//-----------------------------------------------------------------------------
void MsgFunc_SetGlobalParameter(bf_read&msg) {
    char szString[256];
    msg.ReadString(szString, sizeof(szString));
    float szFloat;
    szFloat = msg.ReadFloat();
    CFMODManager::SetGlobalParameter(szString, szFloat);
}

//-----------------------------------------------------------------------------
// Purpose: Start the FMOD Studio System and initialize it
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CFMODManager::StartEngine() {
    pFMODEventListener = new CFMODEventListener(this);
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
    usermessages->HookMessage("FMODLoadBank", MsgFunc_LoadBank);
    usermessages->HookMessage("FMODStartEvent", MsgFunc_StartEvent);
    usermessages->HookMessage("FMODStopEvent", MsgFunc_StopEvent);
    usermessages->HookMessage("FMODSetGlobalParameter", MsgFunc_SetGlobalParameter);
    Log("FMOD Client - Successfully hooked up the UserMessages\n");

    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Stop the FMOD Studio System
// Output: The error code (or 0 if no error was encountered)
//-----------------------------------------------------------------------------
int CFMODManager::StopEngine() {
    Msg("FMOD Client - Stopping Engine\n");
    FMOD_RESULT result;
    result = fmodStudioSystem->release();
    if (result != FMOD_OK) {
        Error("FMOD Client - Engine error! (%d) %s\n", result, FMOD_ErrorString(result));
        return (-1);
    }
    Log("FMOD Client - Engine successfully stopped\n");
    return (0);
}

//-----------------------------------------------------------------------------
// Purpose: Sanitize the name of an FMOD Bank, adding ".bank" if it's not already present
// Input: The FMOD Bank name to sanitize
// Output: The sanitized Bank name (same as the initial if it was already ending with ".bank")
//-----------------------------------------------------------------------------
const char* SanitizeBankName(const char* bankName) {
    const char* bankExtension = ".bank";
    size_t bankNameLength = strlen(bankName);
    size_t bankExtensionLength = strlen(bankExtension);
    if (bankNameLength >= bankExtensionLength && Q_strcmp(bankName + bankNameLength - bankExtensionLength, bankExtension)
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
const char* CFMODManager::GetBankPath(const char* bankName) {
    const char* sanitizedBankName = SanitizeBankName(bankName);
    char* bankPath = new char[512];
    Q_snprintf(bankPath, 512, "%s/sound/fmod/banks/%s", engine->GetGameDirectory(), sanitizedBankName);
    // convert backwards slashes to forward slashes
    for (int i = 0; i < 512; i++) {
        if (bankPath[i] == '\\')
            bankPath[i] = '/';
    }
    return bankPath;
}
