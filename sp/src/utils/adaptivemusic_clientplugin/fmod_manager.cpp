#include "fmod_manager.h"
#include "fmod_studio.hpp"
#include "fmod_errors.h"
#include "convar.h"
#include "usermessages.h"

CFMODManager gFMODManager;

CFMODManager* FMODManager() {
    return &gFMODManager;
}

CFMODManager::CFMODManager()
= default;

CFMODManager::~CFMODManager()
= default;