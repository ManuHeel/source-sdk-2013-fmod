#include "cbase.h"
#include "adaptive_music_system.h"
#include "adaptive_music_watcher.h"

//===========================================================================================================
// BASE WATCHER
//===========================================================================================================

BEGIN_DATADESC(CAdaptiveMusicWatcher)
                    DEFINE_THINKFUNC(WatchThink),
END_DATADESC()

LINK_ENTITY_TO_CLASS(adaptive_music_watcher, CAdaptiveMusicWatcher
);

CAdaptiveMusicWatcher::CAdaptiveMusicWatcher() = default;

void CAdaptiveMusicWatcher::SetAdaptiveMusicSystem(CAdaptiveMusicSystem *pAdaptiveMusicSystemRef) {
    pAdaptiveMusicSystem = pAdaptiveMusicSystemRef;
    pAdaptiveMusicPlayer = pAdaptiveMusicSystem->pAdaptiveMusicPlayer;
}

void CAdaptiveMusicWatcher::SetParameterName(const char *pName) {
    parameterName = pName;
}

void CAdaptiveMusicWatcher::Spawn() {
    CLogicalEntity::Spawn();
    SetThink(&CAdaptiveMusicWatcher::WatchThink);
    SetNextThink(gpGlobals->curtime + 1.0f); // Think at 1Hz
}

void CAdaptiveMusicWatcher::WatchThink() {
    // Base watcher so doing nothing (shouldn't even be created as-is)
    SetNextThink(gpGlobals->curtime + 1.0f); // Think at 1Hz
}

//===========================================================================================================
// HEALTH WATCHER
//===========================================================================================================

BEGIN_DATADESC(CAdaptiveMusicHealthWatcher)
                    DEFINE_THINKFUNC(WatchHealthThink),
END_DATADESC()

LINK_ENTITY_TO_CLASS(adaptive_music_health_watcher, CAdaptiveMusicHealthWatcher
);

CAdaptiveMusicHealthWatcher::CAdaptiveMusicHealthWatcher() {
    lastKnownHealth = 0.0f;
};

void CAdaptiveMusicHealthWatcher::Spawn() {
    CAdaptiveMusicWatcher::Spawn();
    Log("FMOD Health Watcher - Spawning\n");
    SetThink(&CAdaptiveMusicHealthWatcher::WatchHealthThink);
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 10Hz
}

void CAdaptiveMusicHealthWatcher::WatchHealthThink() {
    if (pAdaptiveMusicPlayer != nullptr) {
        auto playerHealth = (float) pAdaptiveMusicPlayer->GetHealth();
        if (playerHealth != lastKnownHealth) {
            lastKnownHealth = playerHealth;
            // Send a FMODSetGlobalParameter usermessage
            CSingleUserRecipientFilter filter(pAdaptiveMusicPlayer);
            filter.MakeReliable();
            UserMessageBegin(filter, "FMODSetGlobalParameter");
            WRITE_STRING(parameterName);
            WRITE_FLOAT(playerHealth);
            MessageEnd();
        }
    }
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 10Hz
}

//===========================================================================================================
// SUIT WATCHER
//===========================================================================================================

BEGIN_DATADESC(CAdaptiveMusicSuitWatcher)
                    DEFINE_THINKFUNC(WatchSuitThink),
END_DATADESC()

LINK_ENTITY_TO_CLASS(adaptive_music_suit_watcher, CAdaptiveMusicSuitWatcher
);

CAdaptiveMusicSuitWatcher::CAdaptiveMusicSuitWatcher() {
    lastKnownSuitStatus = false;
};

void CAdaptiveMusicSuitWatcher::Spawn() {
    CAdaptiveMusicWatcher::Spawn();
    Log("FMOD Suit Watcher - Spawning\n");
    SetThink(&CAdaptiveMusicSuitWatcher::WatchSuitThink);
    SetNextThink(gpGlobals->curtime + 0.2f); // Think at 5Hz
}

void CAdaptiveMusicSuitWatcher::WatchSuitThink() {
    if (pAdaptiveMusicPlayer != nullptr) {
        bool suitStatus = pAdaptiveMusicPlayer->IsSuitEquipped();
        if (suitStatus != lastKnownSuitStatus) {
            lastKnownSuitStatus = suitStatus;
            // Send a FMODSetGlobalParameter usermessage
            CSingleUserRecipientFilter filter(pAdaptiveMusicPlayer);
            filter.MakeReliable();
            UserMessageBegin(filter, "FMODSetGlobalParameter");
            WRITE_STRING(parameterName);
            WRITE_FLOAT(static_cast<float>(suitStatus)); // Sending the boolean as a float to ease FMOD handling
            MessageEnd();
        }
    }
    SetNextThink(gpGlobals->curtime + 0.2f); // Think at 5Hz
}
