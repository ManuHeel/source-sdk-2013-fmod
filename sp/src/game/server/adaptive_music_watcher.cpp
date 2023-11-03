#include "cbase.h"
#include "adaptive_music_system.h"
#include "adaptive_music_watcher.h"

//===========================================================================================================
// ADAPTIVE MUSIC WATCHER LOGICAL ENTITY
//===========================================================================================================

BEGIN_DATADESC(CAdaptiveMusicWatcher)
                    DEFINE_THINKFUNC(WatchThink)
END_DATADESC()

LINK_ENTITY_TO_CLASS(adaptive_music_watcher, CAdaptiveMusicWatcher);

void CAdaptiveMusicWatcher::SetAdaptiveMusicSystem(CAdaptiveMusicSystem *pAdaptiveMusicSystemRef) {
    pAdaptiveMusicSystem = pAdaptiveMusicSystemRef;
}

void CAdaptiveMusicWatcher::Spawn() {
    CLogicalEntity::Spawn();
    Log("FMOD Watcher - Spawning\n");
    SetThink(&CAdaptiveMusicWatcher::WatchThink);
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 10Hz
}

void CAdaptiveMusicWatcher::WatchThink() {
    CBasePlayer *player = pAdaptiveMusicSystem->pAdaptiveMusicPlayer;
    if (player != nullptr) {
        if (true) { // TODO : Replace this with a system asking wherever we need for a healthwatcher
            auto playerHealth = (float) player->GetHealth();
            // Send a FMODSetGlobalParameter usermessage
            CSingleUserRecipientFilter filter(player);
            filter.MakeReliable();
            UserMessageBegin(filter, "FMODSetGlobalParameter");
            WRITE_STRING("health");
            WRITE_FLOAT(playerHealth);
            MessageEnd();
        }
    }
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 10Hz
}
