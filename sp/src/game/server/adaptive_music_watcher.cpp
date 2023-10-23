#include "cbase.h"
#include "adaptive_music_watcher.h"
#include "igamesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//===========================================================================================================
// ADAPTIVE MUSIC WATCHER
//===========================================================================================================

CAdaptiveMusicWatcher::CAdaptiveMusicWatcher() {
}

CAdaptiveMusicWatcher::CAdaptiveMusicWatcher(CAdaptiveMusicSystem *adaptiveMusicSystem) {
}

bool CAdaptiveMusicWatcher::Init() {
    return true;
}

void CAdaptiveMusicWatcher::Update() {
}

CAdaptiveMusicWatcher g_CAdaptiveMusicWatcher;
