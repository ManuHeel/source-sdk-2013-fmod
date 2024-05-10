#ifndef FMOD_MANAGER_H
#define FMOD_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "fmod.hpp"
#include "fmod_studio.hpp"

class CFMODManager {
public:

    FMOD::Studio::System *fmodStudioSystem;
    FMOD::Studio::Bank *loadedFmodStudioBank;
    char *loadedFmodStudioBankName;
    FMOD::Studio::Bank *loadedFmodStudioStringsBank;
    FMOD::Studio::EventDescription *loadedFmodStudioEventDescription;
    char *loadedFmodStudioEventPath;
    FMOD::Studio::EventInstance *createdFmodStudioEventInstance;

    CFMODManager();

    ~CFMODManager();
};

extern CFMODManager *FMODManager();

#endif // FMOD_MANAGER_H