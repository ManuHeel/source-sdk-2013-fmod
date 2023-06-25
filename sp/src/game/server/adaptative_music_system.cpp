//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The system for handling director's commentary style production info in-game.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#ifndef _XBOX
#include "tier0/icommandline.h"
#include "igamesystem.h"
#include "filesystem.h"
#include <KeyValues.h>
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "soundenvelope.h"
#include "utldict.h"
#include "isaverestore.h"
#include "eventqueue.h"
#include "saverestore_utlvector.h"
#include "gamestats.h"
#include "ai_basenpc.h"
#include "Sprite.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//===========================================================================================================
// COMMENTARY GAME SYSTEM
//===========================================================================================================
void CV_GlobalChange_Commentary(IConVar *var, const char *pOldString, float flOldValue);

//-----------------------------------------------------------------------------
// Purpose: Game system to kickstart the director's commentary
//-----------------------------------------------------------------------------
class CCommentarySystem : public CAutoGameSystemPerFrame
{
public:
	DECLARE_DATADESC();

	CCommentarySystem() : CAutoGameSystemPerFrame("CCommentarySystem")
	{
		m_iCommentaryNodeCount = 0;
	}

	virtual void LevelInitPreEntity()
	{
		CalculateCommentaryState();
	}

	void CalculateCommentaryState(void)
	{
		// Set the available cvar if we can find commentary data for this level
		char szFullName[512];
		Q_snprintf(szFullName, sizeof(szFullName), "maps/%s_commentary.txt", STRING(gpGlobals->mapname));
		if (filesystem->FileExists(szFullName))
		{
			commentary_available.SetValue(true);

			// If the user wanted commentary, kick it on
			if (commentary.GetBool())
			{
				g_bInCommentaryMode = true;
			}
		}
		else
		{
			g_bInCommentaryMode = false;
			commentary_available.SetValue(false);
		}
	}

	virtual void LevelShutdownPreEntity()
	{
		ShutDownCommentary();
	}

	void ParseEntKVBlock(CBaseEntity *pNode, KeyValues *pkvNode)
	{
		KeyValues *pkvNodeData = pkvNode->GetFirstSubKey();
		while (pkvNodeData)
		{
			// Handle the connections block
			if (!Q_strcmp(pkvNodeData->GetName(), "connections"))
			{
				ParseEntKVBlock(pNode, pkvNodeData);
			}
			else
			{
#define COMMENTARY_STRING_LENGTH_MAX		1024

				const char *pszValue = pkvNodeData->GetString();
				Assert(Q_strlen(pszValue) < COMMENTARY_STRING_LENGTH_MAX);
				if (Q_strnchr(pszValue, '^', COMMENTARY_STRING_LENGTH_MAX))
				{
					// We want to support quotes in our strings so that we can specify multiple parameters in
					// an output inside our commentary files. We convert ^s to "s here.
					char szTmp[COMMENTARY_STRING_LENGTH_MAX];
					Q_strncpy(szTmp, pszValue, COMMENTARY_STRING_LENGTH_MAX);
					int len = Q_strlen(szTmp);
					for (int i = 0; i < len; i++)
					{
						if (szTmp[i] == '^')
						{
							szTmp[i] = '"';
						}
					}

					pNode->KeyValue(pkvNodeData->GetName(), szTmp);
				}
				else
				{
					pNode->KeyValue(pkvNodeData->GetName(), pszValue);
				}
			}

			pkvNodeData = pkvNodeData->GetNextKey();
		}
	}

	virtual void LevelInitPostEntity(void)
	{
		if (!IsInCommentaryMode())
			return;

		// Don't spawn commentary entities when loading a savegame
		if (gpGlobals->eLoadType == MapLoad_LoadGame || gpGlobals->eLoadType == MapLoad_Background)
			return;

		m_bCommentaryEnabledMidGame = false;
		InitCommentary();

		IGameEvent *event = gameeventmanager->CreateEvent("playing_commentary");
		gameeventmanager->FireEventClientSide(event);
	}

	void InitCommentary(void)
	{
		// Install the global cvar callback
		cvar->InstallGlobalChangeCallback(CV_GlobalChange_Commentary);

		m_flNextTeleportTime = 0;
		m_iTeleportStage = TELEPORT_NONE;
		m_hLastCommentaryNode = NULL;

		// If we find the commentary semaphore, the commentary entities already exist.
		// This occurs when you transition back to a map that has saved commentary nodes in it.
		if (gEntList.FindEntityByName(NULL, COMMENTARY_SPAWNED_SEMAPHORE))
			return;

		// Spawn the commentary semaphore entity
		CBaseEntity *pSemaphore = CreateEntityByName("info_target");
		pSemaphore->SetName(MAKE_STRING(COMMENTARY_SPAWNED_SEMAPHORE));

		bool oldLock = engine->LockNetworkStringTables(false);

		// Find the commentary file
		char szFullName[512];
		Q_snprintf(szFullName, sizeof(szFullName), "maps/%s_commentary.txt", STRING(gpGlobals->mapname));
		KeyValues *pkvFile = new KeyValues("Commentary");
		if (pkvFile->LoadFromFile(filesystem, szFullName, "MOD"))
		{
			Msg("Commentary: Loading commentary data from %s. \n", szFullName);

			// Load each commentary block, and spawn the entities
			KeyValues *pkvNode = pkvFile->GetFirstSubKey();
			while (pkvNode)
			{
				// Get node name
				const char *pNodeName = pkvNode->GetName();

				// Skip the trackinfo
				if (!Q_strncmp(pNodeName, "trackinfo", 9))
				{
					pkvNode = pkvNode->GetNextKey();
					continue;
				}

				KeyValues *pClassname = pkvNode->FindKey("classname");
				if (pClassname)
				{
					// Use the classname instead
					pNodeName = pClassname->GetString();
				}

				// Spawn the commentary entity
				CBaseEntity *pNode = CreateEntityByName(pNodeName);
				if (pNode)
				{
					ParseEntKVBlock(pNode, pkvNode);
					DispatchSpawn(pNode);

					EHANDLE hHandle;
					hHandle = pNode;
					m_hSpawnedEntities.AddToTail(hHandle);

					CPointCommentaryNode *pCommNode = dynamic_cast<CPointCommentaryNode*>(pNode);
					if (pCommNode)
					{
						m_iCommentaryNodeCount++;
						pCommNode->SetNodeNumber(m_iCommentaryNodeCount);
					}
				}
				else
				{
					Warning("Commentary: Failed to spawn commentary entity, type: '%s'\n", pNodeName);
				}

				// Move to next entity
				pkvNode = pkvNode->GetNextKey();
			}

			// Then activate all the entities
			for (int i = 0; i < m_hSpawnedEntities.Count(); i++)
			{
				m_hSpawnedEntities[i]->Activate();
			}
		}
		else
		{
			Msg("Commentary: Could not find commentary data file '%s'. \n", szFullName);
		}

		engine->LockNetworkStringTables(oldLock);
	}

	void ShutDownCommentary(void)
	{
		if (GetActiveNode())
		{
			GetActiveNode()->AbortPlaying();
		}

		// Destroy all the entities created by commentary
		for (int i = m_hSpawnedEntities.Count() - 1; i >= 0; i--)
		{
			if (m_hSpawnedEntities[i])
			{
				UTIL_Remove(m_hSpawnedEntities[i]);
			}
		}
		m_hSpawnedEntities.Purge();
		m_iCommentaryNodeCount = 0;

		// Remove the commentary semaphore
		CBaseEntity *pSemaphore = gEntList.FindEntityByName(NULL, COMMENTARY_SPAWNED_SEMAPHORE);
		if (pSemaphore)
		{
			UTIL_Remove(pSemaphore);
		}

		// Remove our global convar callback
		cvar->RemoveGlobalChangeCallback(CV_GlobalChange_Commentary);

		// Reset any convars that have been changed by the commentary
		for (int i = 0; i < m_ModifiedConvars.Count(); i++)
		{
			ConVar *pConVar = (ConVar *)cvar->FindVar(m_ModifiedConvars[i].pszConvar);
			if (pConVar)
			{
				pConVar->SetValue(m_ModifiedConvars[i].pszOrgValue);
			}
		}
		m_ModifiedConvars.Purge();

		m_hCurrentNode = NULL;
		m_hActiveCommentaryNode = NULL;
		m_hLastCommentaryNode = NULL;
		m_flNextTeleportTime = 0;
		m_iTeleportStage = TELEPORT_NONE;
	}

	void SetCommentaryMode(bool bCommentaryMode)
	{
		g_bInCommentaryMode = bCommentaryMode;
		CalculateCommentaryState();

		// If we're turning on commentary, create all the entities.
		if (IsInCommentaryMode())
		{
			m_bCommentaryEnabledMidGame = true;
			InitCommentary();
		}
		else
		{
			ShutDownCommentary();
		}
	}

private:
	int		m_afPlayersLastButtons;
	int		m_iCommentaryNodeCount;
	bool	m_bCommentaryConvarsChanging;
	int		m_iClearPressedButtons;
	bool	m_bCommentaryEnabledMidGame;
	float	m_flNextTeleportTime;
	int		m_iTeleportStage;

	CUtlVector< modifiedconvars_t > m_ModifiedConvars;
	CUtlVector<EHANDLE>				m_hSpawnedEntities;
	CHandle<CPointCommentaryNode>	m_hCurrentNode;
	CHandle<CPointCommentaryNode>	m_hActiveCommentaryNode;
	CHandle<CPointCommentaryNode>	m_hLastCommentaryNode;
};

CCommentarySystem	g_CommentarySystem;

#else

bool IsInCommentaryMode(void)
{
	return false;
}

#endif
