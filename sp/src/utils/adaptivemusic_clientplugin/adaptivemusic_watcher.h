#ifndef ADAPTIVEMUSIC_WATCHER_H
#define ADAPTIVEMUSIC_WATCHER_H
#ifdef _WIN32
#pragma once
#endif

#include <list>
#include <string>
//#include "adaptivemusic_clientplugin.h"

//===========================================================================================================
// BASE WATCHER
//===========================================================================================================

class CAdaptiveMusicClientPlugin;

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

#endif