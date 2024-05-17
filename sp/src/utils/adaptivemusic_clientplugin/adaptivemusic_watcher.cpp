#include "adaptivemusic_clientplugin.h"
#include "adaptivemusic_watcher.h"

#include <string>

//===========================================================================================================
// BASE WATCHER
//===========================================================================================================

CAdaptiveMusicWatcher::CAdaptiveMusicWatcher() = default;

void CAdaptiveMusicWatcher::SetAdaptiveMusicClientPlugin(CAdaptiveMusicClientPlugin *pAdaptiveMusicClientPluginRef) {
    pAdaptiveMusicClientPlugin = pAdaptiveMusicClientPluginRef;
}

void CAdaptiveMusicWatcher::SetParameterName(const char *pName) {
    parameterName = pName;
}

void CAdaptiveMusicWatcher::Init() {
}

void CAdaptiveMusicWatcher::Think() {
}

//===========================================================================================================
// ZONE WATCHER
//===========================================================================================================

bool IsVectorWithinZone(Vector vector, AdaptiveMusicZone_t zone) {
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

void CAdaptiveMusicZoneWatcher::SetZones(std::list<AdaptiveMusicZone_t> *zonesRef) {
    zones = zonesRef;
}

void CAdaptiveMusicZoneWatcher::Think() {
    if (pAdaptiveMusicClientPlugin != nullptr) {
        edict_t *pPlayer = NULL;
        pPlayer = pAdaptiveMusicClientPlugin->entityinfomanager->FindEntityByClassname(pPlayer, "player");
        Vector pPlayerVector = pPlayer->GetCollideable()->GetCollisionOrigin();
        Log("%f", pPlayerVector.x);
    }
    /*
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
    */
}