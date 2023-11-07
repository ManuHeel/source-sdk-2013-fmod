#include "cbase.h"
#include "baseentity.h"
#include "ai_basenpc.h"

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

//===========================================================================================================
// CHASED WATCHER
//===========================================================================================================

BEGIN_DATADESC(CAdaptiveMusicChasedWatcher)
                    DEFINE_THINKFUNC(WatchChasedThink),
END_DATADESC()

LINK_ENTITY_TO_CLASS(adaptive_music_chased_watcher, CAdaptiveMusicChasedWatcher
);

CAdaptiveMusicChasedWatcher::CAdaptiveMusicChasedWatcher() {
    lastKnownChased = 0.0f;
};

void CAdaptiveMusicChasedWatcher::Spawn() {
    CAdaptiveMusicWatcher::Spawn();
    Log("FMOD Health Watcher - Spawning\n");
    SetThink(&CAdaptiveMusicChasedWatcher::WatchChasedThink);
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 5Hz
}

void CAdaptiveMusicChasedWatcher::WatchChasedThink() {
    if (pAdaptiveMusicPlayer != nullptr) {
        auto chased = static_cast<float>(CAdaptiveMusicChasedWatcher::GetChasedCount());
        if (chased != lastKnownChased) {
            lastKnownChased = chased;
            // Send a FMODSetGlobalParameter usermessage
            CSingleUserRecipientFilter filter(pAdaptiveMusicPlayer);
            filter.MakeReliable();
            UserMessageBegin(filter, "FMODSetGlobalParameter");
            WRITE_STRING(parameterName);
            WRITE_FLOAT(chased);
            MessageEnd();
        }
    }
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 5Hz
}

// Find Enemy Entities
int CAdaptiveMusicChasedWatcher::GetChasedCount() {
    int chasedCount = 0;
    CBaseEntity *pNPC = nullptr;
    if (pAdaptiveMusicPlayer != nullptr) {
        // Get all NPC entites
        while ((pNPC = gEntList.FindEntityByClassname(pNPC, "npc_*")) != nullptr) {
            auto *pAI = dynamic_cast<CAI_BaseNPC *>(pNPC); // TODO: Find if this is the best way to get an NPC's AI, seems convoluted.
            if (pAI) {
                int relationToPlayer = pAI->IRelationType(pAdaptiveMusicPlayer);
                // If the NPC hates the player...
                if (relationToPlayer == D_HT) {
                    // ... and his current ennemy is the player...
                    if (pAI->GetEnemy() != nullptr && pAI->GetEnemy() == pAdaptiveMusicPlayer) {
                        // ...then we consider the player chased/fought by the NPC
                        chasedCount++;
                    }
                    /*
                    // Unused at the moment
                    if (pAI->GetTarget() != nullptr) {
                        const char *targetName = pAI->GetTarget()->GetClassname();
                        Log("Target of npc %s is %s\n", npcName, targetName);
                    }
                    */
                }
            }
        }
    }
    return chasedCount;
}