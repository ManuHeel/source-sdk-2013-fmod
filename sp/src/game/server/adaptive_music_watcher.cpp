#include "cbase.h"
#include "adaptive_music_system.h"
#include "adaptive_music_watcher.h"

//===========================================================================================================
// ADAPTIVE MUSIC WATCHER LOGICAL ENTITY
//===========================================================================================================

BEGIN_DATADESC(CAdaptiveMusicWatcher)
                    DEFINE_THINKFUNC(WatchThink),
                    // Health Watcher
                    DEFINE_FIELD(watchHealth, FIELD_BOOLEAN)
END_DATADESC()

LINK_ENTITY_TO_CLASS(adaptive_music_watcher, CAdaptiveMusicWatcher);

CAdaptiveMusicWatcher::CAdaptiveMusicWatcher() = default;

void CAdaptiveMusicWatcher::SetAdaptiveMusicSystem(CAdaptiveMusicSystem *pAdaptiveMusicSystemRef) {
    pAdaptiveMusicSystem = pAdaptiveMusicSystemRef;
    pAdaptiveMusicPlayer = pAdaptiveMusicSystem->pAdaptiveMusicPlayer;
}

void CAdaptiveMusicWatcher::Spawn() {
    CLogicalEntity::Spawn();
    Log("FMOD Watcher - Spawning\n");
    SetThink(&CAdaptiveMusicWatcher::WatchThink);
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 10Hz
}

void CAdaptiveMusicWatcher::WatchThink() {
    if (pAdaptiveMusicPlayer != nullptr) {
        if (watchHealth) {
            WatchHealth();
        }
    }
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 10Hz
}

void CAdaptiveMusicWatcher::WatchHealth() {
    auto playerHealth = (float) pAdaptiveMusicPlayer->GetHealth();
    if (playerHealth != lastKnownHealth) {
        lastKnownHealth = playerHealth;
        // Send a FMODSetGlobalParameter usermessage
        CSingleUserRecipientFilter filter(pAdaptiveMusicPlayer);
        filter.MakeReliable();
        UserMessageBegin(filter, "FMODSetGlobalParameter");
        WRITE_STRING("health");
        WRITE_FLOAT(playerHealth);
        MessageEnd();
    }
}