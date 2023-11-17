#include "cbase.h"
#include "baseentity.h"
#include "ai_basenpc.h"

#include <algorithm>
#include <string>

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
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 10Hz
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
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 10Hz
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
    enemies = {
            "npc_advisor",
            "npc_antlion",
            "npc_antlionguard",
            "npc_barnacle",
            "npc_breen",
            "npc_clawscanner",
            "npc_combinedropship",
            "npc_combinegunship",
            "npc_fastzombie",
            "npc_fastzombie_torso",
            "npc_headcrab",
            "npc_headcrab_black",
            "npc_headcrab_fast",
            "npc_helicopter",
            "npc_hunter",
            "npc_ichthyosaur",
            "npc_manhack",
            "npc_metropolice",
            "npc_poisonzombie",
            "npc_rollermine",
            "npc_sniper",
            "npc_stalker",
            "npc_strider",
            "npc_turret_ceiling",
            "npc_turret_floor",
            "npc_turret_ground",
            "npc_zombie",
            "npc_zombie_torso",
            "npc_zombine"
    };
    lastKnownChased = 0.0f;
};

void CAdaptiveMusicChasedWatcher::Spawn() {
    CAdaptiveMusicWatcher::Spawn();
    Log("FMOD Chased Watcher - Spawning\n");
    SetThink(&CAdaptiveMusicChasedWatcher::WatchChasedThink);
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 10Hz
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
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 10Hz
}

bool CAdaptiveMusicChasedWatcher::IsEnemy(const char *className) {
    std::string stringClassName(className); // Cast into a C++-style string
    return std::find(enemies.begin(), enemies.end(), className) != enemies.end();
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
                const char *className = pNPC->GetClassname();
                // If the NPC hates the player and could be an enemy...
                if (relationToPlayer == D_HT && className != nullptr && IsEnemy(className)) {
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

//===========================================================================================================
// ENTITY WATCHER
//===========================================================================================================

BEGIN_DATADESC(CAdaptiveMusicEntityWatcher)
                    DEFINE_THINKFUNC(WatchEntityThink),
END_DATADESC()

LINK_ENTITY_TO_CLASS(adaptive_music_entity_watcher, CAdaptiveMusicEntityWatcher
);

CAdaptiveMusicEntityWatcher::CAdaptiveMusicEntityWatcher() {
    lastKnownEntityStatus = false;
};

void CAdaptiveMusicEntityWatcher::SetEntityWatchedStatus(const char *pStatus) {
    watchedEntityStatus = pStatus;
}

void CAdaptiveMusicEntityWatcher::SetEntityClass(const char *pClass) {
    watchedEntityClass = pClass;
}

void CAdaptiveMusicEntityWatcher::Spawn() {
    CAdaptiveMusicWatcher::Spawn();
    Log("FMOD Entity Watcher - Spawning\n");
    SetThink(&CAdaptiveMusicEntityWatcher::WatchEntityThink);
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 10Hz
}

void CAdaptiveMusicEntityWatcher::WatchEntityThink() {
    if (pAdaptiveMusicPlayer != nullptr) {
        auto entityStatusState = static_cast<float>(CAdaptiveMusicEntityWatcher::GetEntityStatusState(watchedEntityClass,watchedEntityStatus));
        if (entityStatusState != lastKnownEntityStatus) {
            lastKnownEntityStatus = entityStatusState;
            // Send a FMODSetGlobalParameter usermessage
            CSingleUserRecipientFilter filter(pAdaptiveMusicPlayer);
            filter.MakeReliable();
            UserMessageBegin(filter, "FMODSetGlobalParameter");
            WRITE_STRING(parameterName);
            WRITE_FLOAT(entityStatusState);
            MessageEnd();
        }
    }
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 10Hz
}

// Find Enemy Entities
bool CAdaptiveMusicEntityWatcher::GetEntityStatusState(const char *entityClass, const char *status) {
    bool entityStatusState = false;
    CBaseEntity *pEntity = gEntList.FindEntityByClassname(NULL, entityClass);
    if (pEntity != nullptr && !Q_strcmp(status, "exists")) {
        entityStatusState = true;
    } else if (pEntity != nullptr && !Q_strcmp(status, "is_alive") && pEntity->IsAlive()) {
        entityStatusState = true;
    }
    return entityStatusState;
}

//===========================================================================================================
// ZONE WATCHER
//===========================================================================================================

BEGIN_DATADESC(CAdaptiveMusicZoneWatcher)
                    DEFINE_THINKFUNC(WatchZoneThink),
END_DATADESC()

LINK_ENTITY_TO_CLASS(adaptive_music_zone_watcher, CAdaptiveMusicZoneWatcher
);

bool IsVectorWithinZone(Vector vector, CAdaptiveMusicSystem::Zone zone) {
    if (
            (vector.x >= zone.minOrigin[0] && vector.x <= zone.maxOrigin[0]) &&
            (vector.y >= zone.minOrigin[1] && vector.y <= zone.maxOrigin[1]) &&
            (vector.z >= zone.minOrigin[2] && vector.z <= zone.maxOrigin[2])) {
        return true;
    }
    return false;
}

CAdaptiveMusicZoneWatcher::CAdaptiveMusicZoneWatcher() {
};

void CAdaptiveMusicZoneWatcher::SetZones(std::list<CAdaptiveMusicSystem::Zone> *zonesRef) {
    zones = zonesRef;
}

void CAdaptiveMusicZoneWatcher::Spawn() {
    CAdaptiveMusicWatcher::Spawn();
    Log("FMOD Zone Watcher - Spawning\n");
    SetThink(&CAdaptiveMusicZoneWatcher::WatchZoneThink);
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 10Hz
}

void CAdaptiveMusicZoneWatcher::WatchZoneThink() {
    if (pAdaptiveMusicPlayer != nullptr) {
        auto currentPosition = pAdaptiveMusicPlayer->GetAbsOrigin();
        for (CAdaptiveMusicSystem::Zone &zone: *zones) {
            bool zoneStatus = IsVectorWithinZone(currentPosition, zone);
            if (zoneStatus != zone.lastKnownZoneStatus) {
                zone.lastKnownZoneStatus = zoneStatus;
                // Send a FMODSetGlobalParameter usermessage
                CSingleUserRecipientFilter filter(pAdaptiveMusicPlayer);
                filter.MakeReliable();
                UserMessageBegin(filter, "FMODSetGlobalParameter");
                WRITE_STRING(zone.parameterName);
                WRITE_FLOAT(zoneStatus);
                MessageEnd();
            }
        }
    }
    SetNextThink(gpGlobals->curtime + 0.1f); // Think at 10Hz
}