//
// MQ2Camera.cpp
// written by brainiac
// https://github.com/brainiac/MQ2Camera
//

#include "../MQ2Plugin.h"

PreSetup("MQ2Camera");

#define PLUGIN_MSG "\ag[MQ2Camera]\ax "

#include <algorithm>


namespace
{
	float s_origZoomCameraMaxDistance = 0.0;
	float s_origUserCameraMaxDistance = 0.0;

	bool s_initialized = false;

	//.text:006646B1 loc_6646B1:                             ; CODE XREF: CEverQuest::MouseWheelScrolled(int)+4D↑j
	//.text:006646B1                 mov     eax, dword_F32A3C
	//.text:006646B6                 fld     dword ptr [eax+34h]
	//.text:006646B9                 fstp    [esp+20h+var_1C]
	//.text:006646BD                 fld     ds:flt_ABEFC4
	//.text:006646C3                 fstp    [esp+20h+var_18]
	//.text:006646C7                 fld     ds:float const g_fMaxZoomCameraDistance <--
	//.text:006646CD                 fstp    [esp+20h+var_14]
	//.text:006646D1                 call    sub_6F7F50
	//.text:006646D6                 push    dword ptr [eax+868h]
	//.text:006646DC                 call    sub_986DD0
	//.text:006646E1                 add     esp, 4
	//.text:006646E4                 test    al, al
	//.text:006646E6                 jz      short loc_6646F2
	//.text:006646E8                 fld     ds:flt_AD1D1C
	//.text:006646EE                 fstp    [esp+20h+var_14]

	// 0x6646b1 02-21-2018
	const char* ZoomCameraMaxDistance_Mask = "x????xxxxxxxxx????xxxxxx????xxxxx????";
	const unsigned char* ZoomCameraMaxDistance_Pattern = (const unsigned char*)"\xA1\x00\x00\x00\x00\xD9\x40\x34\xD9\x5C\x24\x04\xD9\x05\x00\x00\x00\x00\xD9\x5C\x24\x08\xD9\x05\x00\x00\x00\x00\xD9\x5C\x24\x0C\xE8\x00\x00\x00\x00";
	const int ZoomCameraMaxDistance_Offset = (0x6646C7 + 2) - 0x6646B1;
	float* ZoomCameraMaxDistance = 0;

	//.text:008E8CB0 public: virtual void __thiscall EQChaseCamera::UpdateCamera(class PlayerClient *) proc near
	//.text:008E8CB0                 mov     eax, CEverQuest * EverQuestObject
	//.text:008E8CB5                 sub     esp, 2Ch
	//.text:008E8CB8                 cmp     dword ptr [eax+5C8h], 5
	//.text:008E8CBF                 push    esi
	//.text:008E8CC0                 push    edi
	//.text:008E8CC1                 mov     edi, [esp+34h+arg_0]
	//.text:008E8CC5                 mov     esi, ecx
	//.text:008E8CC7                 jnz     short loc_8E8D0E
	//.text:008E8CC9                 fld     ds:float const g_fMaxCameraDistance <--
	//.text:008E8CCF                 fcom    dword ptr [esi+34h]
	//.text:008E8CD2                 fnstsw  ax
	//.text:008E8CD4                 test    ah, 5
	//.text:008E8CD7                 jp      short loc_8E8CDE
	//.text:008E8CD9                 fstp    dword ptr [esi+34h]
	//.text:008E8CDC                 jmp     short loc_8E8CE0

	// 0x8e8cb0 02-21-2018
	const char* UserCameraMaxDistance_Mask = "x????xxxxx?????xxxxxxxxxxxx????xxx";
	const unsigned char* UserCameraMaxDistance_Pattern = (const unsigned char*)"\xA1\x00\x00\x00\x00\x83\xEC\x2C\x83\xB8\x00\x00\x00\x00\x00\x56\x57\x8B\x7C\x24\x38\x8B\xF1\x75\x45\xD9\x05\x00\x00\x00\x00\xD8\x56\x34";
	const int UserCameraMaxDistance_Offset = (0x8E8CC9 + 2) - 0x8E8CB0;
	float* UserCameraMaxDistance = 0;

	//.text:005C5B20 public: void __thiscall CDisplay::SetViewActor(class CActorInterface *) proc near
	//.text:005C5B20                 sub     esp, 18h
	//.text:005C5B23                 push    esi
	//.text:005C5B24                 mov     esi, ecx
	//.text:005C5B26                 mov     ecx, CActorInterface * ViewActor
	//.text:005C5B2C                 test    ecx, ecx
	//.text:005C5B2E                 jz      short loc_5C5B37
	//.text:005C5B30                 mov     eax, [ecx]
	//.text:005C5B32                 push    0
	//.text:005C5B34                 call    dword ptr [eax+34h]
	//.text:005C5B37
	//.text:005C5B37 loc_5C5B37:                             ; CODE XREF: CDisplay::SetViewActor(CActorInterface *)+E↑j
	//.text:005C5B37                 mov     ecx, [esp+1Ch+arg_0]
	//.text:005C5B3B                 mov     CActorInterface * ViewActor, ecx
	//.text:005C5B41                 test    ecx, ecx
	//.text:005C5B43                 jz      short loc_5C5BB2
	//.text:005C5B45                 mov     eax, [ecx]
	//.text:005C5B47                 lea     edx, [esp+1Ch+var_18]
	//.text:005C5B4B                 push    edx
	//.text:005C5B4C                 call    dword ptr [eax+80h]

	// 0x5c5b20 02-21-2018
	const char* SetViewActor_Mask = "xxxxxxxx????xxxxxxxxxxxxxxxxx????xxxxxxxxxx";
	const unsigned char* SetViewActor_Pattern = (const unsigned char*)"\x83\xEC\x18\x56\x8B\xF1\x8B\x0D\x00\x00\x00\x00\x85\xC9\x74\x07\x8B\x01\x6A\x00\xFF\x50\x34\x8B\x4C\x24\x20\x89\x0D\x00\x00\x00\x00\x85\xC9\x74\x6D\x8B\x01\x8D\x54\x24\x04";

	DWORD CDisplay__SetViewActor_Offset = 0;
}

class CActorInterface;
class CDisplay_MQ2Camera_Extension : public CDisplay
{
public:
	void SetViewActor(CActorInterface* viewActor);
};

FUNCTION_AT_ADDRESS(void CDisplay_MQ2Camera_Extension::SetViewActor(CActorInterface*), CDisplay__SetViewActor_Offset);

inline bool DataCompare(const unsigned char* pData, const unsigned char* bMask, const char* szMask)
{
	for (; *szMask; ++szMask, ++pData, ++bMask)
	{
		if (*szMask == 'x' && *pData != *bMask)
			return false;
	}
	return (*szMask) == 0;
}

unsigned long FindPattern(unsigned long dwAddress, unsigned long dwLen, const unsigned char* bMask, const char* szMask)
{
	for (unsigned long i = 0; i < dwLen; i++)
	{
		if (DataCompare((unsigned char*)(dwAddress + i), bMask, szMask))
			return (unsigned long)(dwAddress + i);
	}

	return 0;
}

template <typename T>
T* GetDataPtrAtOffset(DWORD address, int offset)
{
	if (address)
	{
		__try {
			address += offset;
			return reinterpret_cast<T*>(*(DWORD*)address);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
		}
	}

	return 0;
}

bool InitValues()
{
	DWORD address = FindPattern(FixOffset(0x600000), 0x100000,
		ZoomCameraMaxDistance_Pattern, ZoomCameraMaxDistance_Mask);
	ZoomCameraMaxDistance = GetDataPtrAtOffset<float>(address, ZoomCameraMaxDistance_Offset);

	if (!ZoomCameraMaxDistance) {
		WriteChatf(PLUGIN_MSG "\arFailed to find [ZoomCameraMaxDistance]\ax");
		return false;
	}

	address = FindPattern(FixOffset(0x800000), 0x100000,
		UserCameraMaxDistance_Pattern, UserCameraMaxDistance_Mask);
	UserCameraMaxDistance = GetDataPtrAtOffset<float>(address, UserCameraMaxDistance_Offset);

	if (!UserCameraMaxDistance) {
		WriteChatf(PLUGIN_MSG "\arFailed to find [UserCameraMaxDistance]\ax");
		return false;
	}

	if ((CDisplay__SetViewActor_Offset = FindPattern(FixOffset(0x500000), 0x100000,
		SetViewActor_Pattern, SetViewActor_Mask)) == 0) {
		WriteChatf(PLUGIN_MSG "\arFailed to find [SetViewActor]\ax");
		return false;
	}

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

void AttachCameraToSpawn(PSPAWNINFO pSpawn)
{
	bool attaching = true;

	if (pSpawn == nullptr)
	{
		// if no spawn is provided, reset to the current player
		pSpawn = GetCharInfo() ? GetCharInfo()->pSpawn : nullptr;
		attaching = false;
	}

	if (!pSpawn)
		return;

	// we need to get the actorinterface for the spawn. This is the pactorex
	// field of ActorClient
	CActorInterface* actor = (CActorInterface*)pSpawn->mActorClient.pcactorex;

	if (actor != nullptr)
	{
		CDisplay_MQ2Camera_Extension* pDisplayEx =
			(CDisplay_MQ2Camera_Extension*)pDisplay;
		pDisplayEx->SetViewActor(actor);

		s_attachedCamera = attaching;

		if (attaching)
		{
			WriteChatf(PLUGIN_MSG "Attaching camera to \ay%s", pSpawn->Name);
		}
	}
}

VOID Cmd_Camera(PSPAWNINFO pChar, PCHAR szLine)
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
		bool reset = !_stricmp(Command, "reset");
		if (reset)
			ResetCameraDistance();
		else
			SetCameraDistance(static_cast<float>(atof(Param)));

		// check to see if we want to save
		GetArg(Command, szLine, 3);
		if (!_stricmp(Command, "save"))
		{
			SaveCameraDistance(reset ? 0.0f : static_cast<float>(atof(Param)));
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
				AttachCameraToSpawn((PSPAWNINFO)pTarget);
			else
				WriteChatf(PLUGIN_MSG "\arNo target to attach camera to");
		}
		// /camera attach id #
		else if (!_stricmp(Param, "id"))
		{
			CHAR Id[MAX_STRING];
			GetArg(Id, szLine, 3);

			PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByID(atoi(Id));
			if (pSpawn)
				AttachCameraToSpawn(pSpawn);
			else
				WriteChatf(PLUGIN_MSG "\arCould not find spawn id '%s'", Id);
		}
		// /camera attach name_of_some_spawn
		else if (strlen(Param) > 0)
		{
			// This behavior simulates SetViewActorByName
			PSPAWNINFO pSpawn = (PSPAWNINFO)GetSpawnByName(Param);
			if (!pSpawn)
				pSpawn = (PSPAWNINFO)GetSpawnByPartialName(Param);
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

PLUGIN_API VOID InitializePlugin(VOID)
{
	WriteChatf(PLUGIN_MSG "v1.0 by brainiac (\aohttps://github.com/brainiac/MQ2Camera\ax)");

	if (!InitValues()) {
		WriteChatf(PLUGIN_MSG "\arFailed to initialize offsets. Plugin will not function.");
		EzCommand("/timed 1 /plugin mq2camera unload");
	}
	else {
		AddCommand("/camera", Cmd_Camera);
		AddDetour((DWORD)ZoomCameraMaxDistance);
		AddDetour((DWORD)UserCameraMaxDistance);
		s_initialized = true;

		WriteChatf(PLUGIN_MSG "Type \ag/camera\ax for more information");

		float distance = LoadCameraDistance();
		if (distance > 1.0)
			SetCameraDistance(distance);
	}
}

PLUGIN_API VOID ShutdownPlugin(VOID)
{
	if (s_initialized) {
		RemoveCommand("/camera");
		ResetCameraDistance();
		RemoveDetour((DWORD)ZoomCameraMaxDistance);
		RemoveDetour((DWORD)UserCameraMaxDistance);
	}
}
