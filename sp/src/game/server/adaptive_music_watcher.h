#ifndef ADAPTIVE_MUSIC_WATCHER_H
#define ADAPTIVE_MUSIC_WATCHER_H
#ifdef _WIN32
#pragma once
#endif

#include <list>
#include <string>
#include "cbase.h"

//===========================================================================================================
// BASE WATCHER
//===========================================================================================================
class CAdaptiveMusicWatcher : public CLogicalEntity {

protected:
    CAdaptiveMusicSystem *pAdaptiveMusicSystem{};
    CBasePlayer *pAdaptiveMusicPlayer{};
    const char *parameterName;

public:
    DECLARE_CLASS(CAdaptiveMusicWatcher, CLogicalEntity);
    DECLARE_DATADESC();

    explicit CAdaptiveMusicWatcher();

    void SetAdaptiveMusicSystem(CAdaptiveMusicSystem *pAdaptiveMusicSystemRef);

    void SetParameterName(const char *pName);

    void Spawn() override;

    void WatchThink();

private:

};

//===========================================================================================================
// HEALTH WATCHER
//===========================================================================================================
class CAdaptiveMusicHealthWatcher : public CAdaptiveMusicWatcher {

protected:
    // Health watcher
    float lastKnownHealth;

public:
    DECLARE_CLASS(CAdaptiveMusicHealthWatcher, CAdaptiveMusicWatcher);
    DECLARE_DATADESC();

    explicit CAdaptiveMusicHealthWatcher();

    void Spawn() override;

    void WatchHealthThink();

private:

};

//===========================================================================================================
// SUIT WATCHER
//===========================================================================================================
class CAdaptiveMusicSuitWatcher : public CAdaptiveMusicWatcher {

protected:
    // Suit watcher
    bool lastKnownSuitStatus;

public:
    DECLARE_CLASS(CAdaptiveMusicHealthWatcher, CAdaptiveMusicWatcher);
    DECLARE_DATADESC();

    explicit CAdaptiveMusicSuitWatcher();

    void Spawn() override;

    void WatchSuitThink();

private:

};

//===========================================================================================================
// CHASED WATCHER
//===========================================================================================================
class CAdaptiveMusicChasedWatcher : public CAdaptiveMusicWatcher {

protected:
    // Chased watcher
    std::vector<std::string> enemies;
    float lastKnownChased;

public:
    DECLARE_CLASS(CAdaptiveMusicChasedWatcher, CAdaptiveMusicWatcher);
    DECLARE_DATADESC();

    explicit CAdaptiveMusicChasedWatcher();

    void Spawn() override;

    void WatchChasedThink();

    bool IsEnemy(const char *className);

    int GetChasedCount();

private:

};

//===========================================================================================================
// ENTITY WATCHER
//===========================================================================================================
class CAdaptiveMusicEntityWatcher : public CAdaptiveMusicWatcher {

protected:
    // Entity watcher
    const char *watchedEntityClass;
    const char *watchedEntityStatus;
    const char *watchedEntityScene;
    float lastKnownEntityStatus;

public:
    DECLARE_CLASS(CAdaptiveMusicEntityWatcher, CAdaptiveMusicWatcher);
    DECLARE_DATADESC();

    explicit CAdaptiveMusicEntityWatcher();

    void SetEntityClass(const char *pClass);

    void SetEntityWatchedStatus(const char *pClass);

    void SetEntityWatchedScene(const char *pClass);

    void Spawn() override;

    void WatchEntityThink();

    bool GetEntityStatusState(const char *entityClass, const char *status);

private:

};

//===========================================================================================================
// ZONE WATCHER
//===========================================================================================================
class CAdaptiveMusicZoneWatcher : public CAdaptiveMusicWatcher {

protected:
    // Zone watcher
    std::list<CAdaptiveMusicSystem::Zone> *zones;

public:
    DECLARE_CLASS(CAdaptiveMusicZoneWatcher, CAdaptiveMusicWatcher);
    DECLARE_DATADESC();

    explicit CAdaptiveMusicZoneWatcher();

    void SetZones(std::list<CAdaptiveMusicSystem::Zone> *zonesRef);

    void Spawn() override;

    void WatchZoneThink();

private:

};

//===========================================================================================================
// SCENE WATCHER
//===========================================================================================================
class CAdaptiveMusicSceneWatcher : public CAdaptiveMusicWatcher {

protected:
    // LCS watcher
    std::list<CAdaptiveMusicSystem::Scene> *scenes;

public:
    DECLARE_CLASS(CAdaptiveMusicSceneWatcher, CAdaptiveMusicWatcher);
    DECLARE_DATADESC();

    explicit CAdaptiveMusicSceneWatcher();

    void SetScenes(std::list<CAdaptiveMusicSystem::Scene> *scenesRef);

    void Spawn() override;

    void WatchSceneThink();

private:

};

#endif //ADAPTIVE_MUSIC_WATCHER_H