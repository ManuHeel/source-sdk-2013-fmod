#ifndef ADAPTIVEMUSIC_WATCHER_H
#define ADAPTIVEMUSIC_WATCHER_H
#ifdef _WIN32
#pragma once
#endif

#include <list>
#include <string>

//===========================================================================================================
// BASE WATCHER
//===========================================================================================================

// Forward declarations
class CAdaptiveMusicClientPlugin;
struct AdaptiveMusicZone_t;
struct AdaptiveMusicScene_t;

class CAdaptiveMusicWatcher {

protected:
    CAdaptiveMusicClientPlugin *pAdaptiveMusicClientPlugin{};
    const char *parameterName;

public:

    explicit CAdaptiveMusicWatcher();

    void SetAdaptiveMusicClientPlugin(CAdaptiveMusicClientPlugin *pAdaptiveMusicClientPluginRef);

    void SetParameterName(const char *pName);

    void Init();

    void Think();

private:

};

//===========================================================================================================
// ZONE WATCHER
//===========================================================================================================
class CAdaptiveMusicZoneWatcher : public CAdaptiveMusicWatcher {

protected:
    // Zone watcher
    std::list<AdaptiveMusicZone_t> *zones;

public:

    explicit CAdaptiveMusicZoneWatcher();

    void SetZones(std::list<AdaptiveMusicZone_t> *zonesRef);

    void Think();

private:

};


#endif