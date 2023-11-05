#ifndef ADAPTIVE_MUSIC_WATCHER_H
#define ADAPTIVE_MUSIC_WATCHER_H
#ifdef _WIN32
#pragma once
#endif

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

#endif //ADAPTIVE_MUSIC_WATCHER_H