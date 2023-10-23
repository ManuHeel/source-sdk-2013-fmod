#include "cbase.h"
#include "adaptive_music_system.h"
#include "adaptive_music_watcher.h"
#include "adaptive_music_healthwatcher.h"
#include "igamesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//===========================================================================================================
// ADAPTIVE MUSIC HEALTH WATCHER
//===========================================================================================================

CAdaptiveMusicHealthWatcher::CAdaptiveMusicHealthWatcher() {
}

CAdaptiveMusicHealthWatcher::CAdaptiveMusicHealthWatcher(CAdaptiveMusicSystem *adaptiveMusicSystem) {
}

bool CAdaptiveMusicHealthWatcher::Init() {
    Msg("FMOD HealthWatcher - Starting a new HealthWatcher\n");
    CAdaptiveMusicSystem/
    SetThink(&CAdaptiveMusicHealthWatcher::Update);
    SetNextThink(gpGlobals->curtime + 1.0f);
    return true;
}

void CAdaptiveMusicHealthWatcher::Update() {
    Log("I love health\n");
}

CAdaptiveMusicHealthWatcher g_AdaptiveMusicHealthWatcher;
