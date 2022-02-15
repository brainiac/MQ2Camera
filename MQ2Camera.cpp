//
// MQ2Camera.cpp
// written by brainiac
// https://github.com/brainiac/MQ2Camera
//

#if !defined(NOMINMAX)
#define NOMINMAX
#endif

#include <mq/Plugin.h>

#include <algorithm>

PreSetup("MQ2Camera");
PLUGIN_VERSION(1.0);

#define PLUGIN_MSG "\ag[MQ2Camera]\ax "

namespace
{
	float s_origZoomCameraMaxDistance = 0.0;
	float s_origUserCameraMaxDistance = 0.0;

	bool s_initialized = false;

	float* ZoomCameraMaxDistance = 0;
	float* UserCameraMaxDistance = 0;
}

bool InitValues()
{
	UserCameraMaxDistance = (float *)FixOffset(__gfMaxCameraDistance_x);
	ZoomCameraMaxDistance = (float *)FixOffset(__gfMaxZoomCameraDistance_x);

	s_origUserCameraMaxDistance = *UserCameraMaxDistance;
	s_origZoomCameraMaxDistance = *ZoomCameraMaxDistance;

	return true;
}

void SetCameraValue(float* cameraValue, float value)
{
	DWORD oldperm;
	VirtualProtectEx(GetCurrentProcess(), (LPVOID)cameraValue, 4, PAGE_EXECUTE_READWRITE, &oldperm);
	*cameraValue = value;
	VirtualProtectEx(GetCurrentProcess(), (LPVOID)cameraValue, 4, oldperm, &oldperm);
}

void ResetCameraDistance()
{
	if (s_initialized)
	{
		SetCameraValue(ZoomCameraMaxDistance, s_origZoomCameraMaxDistance);
		SetCameraValue(UserCameraMaxDistance, s_origUserCameraMaxDistance);
	}
}

void SetCameraDistance(float distance)
{
	if (s_initialized)
	{
		float zoomValue = std::max(distance, s_origZoomCameraMaxDistance);
		SetCameraValue(ZoomCameraMaxDistance, zoomValue);

		float userValue = std::max(distance, s_origUserCameraMaxDistance);
		SetCameraValue(UserCameraMaxDistance, userValue);

		WriteChatf(PLUGIN_MSG "Camera max distance set to \ay%.2f\ax", distance);
	}
}

void SaveCameraDistance(float distance)
{
	char szValue[256];
	sprintf_s(szValue, "%.2f", distance);

	WritePrivateProfileString("MQ2Camera", "MaxDistance", szValue, INIFileName);
}

float LoadCameraDistance()
{
	char szValue[256] = { 0 };

	GetPrivateProfileString("MQ2Camera", "MaxDistance", "", szValue, 256, INIFileName);

	float fValue = 0.0;
	sscanf_s(szValue, "%f", &fValue);

	return fValue;
}

bool s_attachedCamera = false;

void AttachCameraToSpawn(PlayerClient* pSpawn)
{
	bool attaching = true;

	if (!pSpawn)
	{
		// if no spawn is provided, reset to the current player
		pSpawn = pLocalPlayer;
		attaching = false;
	}

	if (!pSpawn)
		return;

	// we need to get the actorinterface for the spawn. This is the pactorex
	// field of ActorClient
	if (pSpawn->mActorClient.pActor)
	{
		pDisplay->SetViewActor(pSpawn->mActorClient.pActor);

		s_attachedCamera = attaching;

		if (attaching)
		{
			WriteChatf(PLUGIN_MSG "Attaching camera to \ay%s", pSpawn->Name);
		}
	}
}

void Cmd_Camera(PlayerClient* pChar,  char* szLine)
{
	if (!s_initialized)
	{
		WriteChatf(PLUGIN_MSG "Plugin is disabled due to initialization error.");
		return;
	}

	CHAR Command[MAX_STRING];
	GetArg(Command, szLine, 1);

	CHAR Param[MAX_STRING];
	GetArg(Param, szLine, 2);

	if (!_stricmp(Command, "distance"))
	{
		const bool reset = !_stricmp(Command, "reset");
		if (reset)
			ResetCameraDistance();
		else
			SetCameraDistance(GetFloatFromString(Param, 0));

		// check to see if we want to save
		GetArg(Command, szLine, 3);
		if (!_stricmp(Command, "save"))
		{
			SaveCameraDistance(reset ? 0.0f : GetFloatFromString(Param, 0));
		}
	}
	else if (!_stricmp(Command, "info"))
	{
		WriteChatf(PLUGIN_MSG "Zoom camera max distance: \ay%.2f\ax (default: %.2f)",
			*ZoomCameraMaxDistance, s_origZoomCameraMaxDistance);
		WriteChatf(PLUGIN_MSG "User camera max distance: \ay%.2f\ax (default: %.2f)",
			*UserCameraMaxDistance, s_origUserCameraMaxDistance);
	}
	else if (!_stricmp(Command, "attach"))
	{
		// /camera attach target
		if (!_stricmp(Param, "target"))
		{
			if (pTarget)
				AttachCameraToSpawn(pTarget);
			else
				WriteChatf(PLUGIN_MSG "\arNo target to attach camera to");
		}
		// /camera attach id #
		else if (!_stricmp(Param, "id"))
		{
			CHAR Id[MAX_STRING];
			GetArg(Id, szLine, 3);

			PlayerClient* pSpawn = GetSpawnByID(GetIntFromString(Id, 0));
			if (pSpawn)
				AttachCameraToSpawn(pSpawn);
			else
				WriteChatf(PLUGIN_MSG "\arCould not find spawn id '%s'", Id);
		}
		// /camera attach name_of_some_spawn
		else if (strlen(Param) > 0)
		{
			// This behavior simulates SetViewActorByName
			PlayerClient* pSpawn = GetSpawnByName(Param);
			if (!pSpawn)
				pSpawn = GetSpawnByPartialName(Param);
			if (pSpawn)
				AttachCameraToSpawn(pSpawn);
			else
				WriteChatf(PLUGIN_MSG "\arCould not find spawn named '%s'", Param);
		}
		else
		{
			WriteChatf(PLUGIN_MSG "Resetting camera");
			AttachCameraToSpawn(nullptr);
		}
	}
	else if (!_stricmp(Command, "detach") || !_stricmp(Command, "reset"))
	{
		// reset the camera
		WriteChatf(PLUGIN_MSG "Resetting camera");
		AttachCameraToSpawn(nullptr);
	}
	else
	{
		WriteChatf(PLUGIN_MSG "Usage:");
		WriteChatf(PLUGIN_MSG "\ag/camera distance [ reset | <distance> ] [ save ]\ax - set camera max distance or reset to default. Specify 'save' to save the value");
		WriteChatf(PLUGIN_MSG "\ag/camera info\ax - report current camera information");
		WriteChatf(PLUGIN_MSG "\ag/camera attach [ target | id # | <spawn name> ]\ax - attach camera to another spawn");
		WriteChatf(PLUGIN_MSG "\ag/camera detach\ax - Reset camera attachment");
	}
}

PLUGIN_API void InitializePlugin()
{
	WriteChatf(PLUGIN_MSG "v%.2f by brainiac (\aohttps://github.com/brainiac/MQ2Camera\ax)", MQ2Version);

	if (!InitValues()) {
		WriteChatf(PLUGIN_MSG "\arFailed to initialize offsets. Plugin will not function.");
		EzCommand("/timed 1 /plugin mq2camera unload");
	}
	else
	{
		AddCommand("/camera", Cmd_Camera);

		//AddDetour(ZoomCameraMaxDistance);
		//AddDetour(UserCameraMaxDistance);
		s_initialized = true;

		WriteChatf(PLUGIN_MSG "Type \ag/camera\ax for more information");

		float distance = LoadCameraDistance();
		if (distance > 1.0)
			SetCameraDistance(distance);
	}
}

PLUGIN_API void ShutdownPlugin()
{
	if (s_initialized)
	{
		RemoveCommand("/camera");
		ResetCameraDistance();

		//RemoveDetour((uintptr_t)ZoomCameraMaxDistance);
		//RemoveDetour((uintptr_t)UserCameraMaxDistance);
	}
}
