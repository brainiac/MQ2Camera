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

	//.text:00551602 loc_551602:                             ; CODE XREF: CEverQuest::MouseWheelScrolled(int)+50j
	//.text:00551602                 mov     eax, dword_F3B5CC
	//.text:00551607                 fld     dword ptr [eax+34h]
	//.text:0055160A                 fstp    [esp+20h+var_1C]
	//.text:0055160E                 fld     ds:flt_AA9D9C
	//.text:00551614                 fstp    [esp+20h+var_18]
	//.text:00551618                 fld     ds:float const g_fMaxZoomCameraDistance
	//.text:0055161E                 fstp    [esp+20h+var_14]
	//.text:00551622                 call    sub_5EAAF0
	//.text:00551627                 mov     eax, [eax+868h]
	//.text:0055162D                 push    eax
	//.text:0055162E                 call    sub_877D00
	//.text:00551633                 add     esp, 4
	//.text:00551636                 test    al, al
	//.text:00551638                 jz      short loc_551644
	//.text:0055163A                 fld     ds:flt_AB9D1C
	//.text:00551640                 fstp    [esp+20h+var_14]

	// 00AB9D20 12-14-2016
	const char* ZoomCameraMaxDistance_Mask = "x????xxxxxxxxx????xxxxxx????xxxxx????xxxxxxxx????xxxxxx?xx????xxxx";
	const unsigned char* ZoomCameraMaxDistance_Pattern = (const unsigned char*)"\xa1\x00\x00\x00\x00\xd9\x40\x34\xd9\x5c\x24\x04\xd9\x05\x00\x00\x00\x00\xd9\x5c\x24\x08\xd9\x05\x00\x00\x00\x00\xd9\x5c\x24\x0c\xe8\x00\x00\x00\x00\x8b\x80\x68\x08\x00\x00\x50\xe8\x00\x00\x00\x00\x83\xc4\x04\x84\xc0\x74\x00\xd9\x05\x00\x00\x00\x00\xd9\x5c\x24\x0c";
	const int ZoomCameraMaxDistance_Offset = (0x551618 + 2) - 0x551602;
	float* ZoomCameraMaxDistance = 0;

	//.text:007DF530 public: virtual void __thiscall EQChaseCamera::UpdateCamera(class PlayerClient *) proc near
	//.text:007DF530                 mov     eax, CEverQuest * EverQuestObject
	//.text:007DF535                 sub     esp, 28h
	//.text:007DF538                 cmp     dword ptr [eax+5C8h], 5
	//.text:007DF53F                 push    ebx
	//.text:007DF540                 push    ebp
	//.text:007DF541                 push    esi
	//.text:007DF542                 mov     esi, [esp+34h+arg_0]
	//.text:007DF546                 push    edi
	//.text:007DF547                 mov     edi, ecx
	//.text:007DF549                 jnz     short loc_7DF590
	//.text:007DF54B                 fld     ds:float const g_fMaxCameraDistance <--
	//.text:007DF551                 fcom    dword ptr [edi+34h]
	//.text:007DF554                 fnstsw  ax
	//.text:007DF556                 test    ah, 5
	//.text:007DF559                 jp      short loc_7DF560
	//.text:007DF55B                 fstp    dword ptr [edi+34h]
	//.text:007DF55E                 jmp     short loc_7DF562

	// 0xAFDEF0 12-09-2015
	const char* UserCameraMaxDistance_Mask = "x????xxxxxxxxxxxxxxxxxxxxx?xx????xxxxxxxxx?xxxx?";
	const unsigned char* UserCameraMaxDistance_Pattern = (const unsigned char*)"\xa1\x00\x00\x00\x00\x83\xec\x28\x83\xb8\xc8\x05\x00\x00\x05\x53\x55\x56\x8b\x74\x24\x38\x57\x8b\xf9\x75\x00\xd9\x05\x00\x00\x00\x00\xd8\x57\x34\xdf\xe0\xf6\xc4\x05\x7a\x00\xd9\x5f\x34\xeb\x00";
	const int UserCameraMaxDistance_Offset = (0x7DF54B + 2) - 0x7DF530;
	float* UserCameraMaxDistance = 0;

	//.text:00463CA0     public: void __thiscall CDisplay::SetViewActor(class CActorInterface *) proc near
	//.text:004B2270                 sub     esp, 18h
	//.text:004B2273                 push    esi
	//.text:004B2274                 mov     esi, ecx
	//.text:004B2276                 mov     ecx, CActorInterface * ViewActor
	//.text:004B227C                 test    ecx, ecx
	//.text:004B227E                 jz      short loc_4B2289
	//.text:004B2280                 mov     eax, [ecx]
	//.text:004B2282                 mov     edx, [eax+34h]
	//.text:004B2285                 push    0
	//.text:004B2287                 call    edx
	//.text:004B2289
	//.text:004B2289 loc_4B2289:                             ; CODE XREF: CDisplay::SetViewActor(CActorInterface *)+Ej
	//.text:004B2289                 mov     ecx, [esp+1Ch+arg_0]
	//.text:004B228D                 mov     CActorInterface * ViewActor, ecx
	//.text:004B2293                 test    ecx, ecx
	//.text:004B2295                 jz      short loc_4B230C
	//.text:004B2297                 mov     eax, [ecx]
	//.text:004B2299                 mov     eax, [eax+80h]
	//.text:004B229F                 lea     edx, [esp+1Ch+var_18]
	//.text:004B22A3                 push    edx
	//.text:004B22A4                 call    eax
	//.text:004B22A6                 mov     ecx, CActorInterface * ViewActor
	//.text:004B22AC                 mov     edx, [ecx]
	//.text:004B22AE                 mov     edx, [edx+84h]
	//.text:004B22B4                 lea     eax, [esp+1Ch+var_C]
	//.text:004B22B8                 push    eax
	//.text:004B22B9                 call    edx

	// 0x463CA0 12-22-2016
	const char* SetViewActor_Mask = "xxxxxxxx????xxx?xxxxxxxxxxxxxxx????xxx?xxxxxxxxxxxxxxxxx????xxxxxxxxxxxxxxx";
	const unsigned char* SetViewActor_Pattern = (const unsigned char*)"\x83\xec\x18\x56\x8b\xf1\x8b\x0d\x00\x00\x00\x00\x85\xc9\x74\x00\x8b\x01\x8b\x50\x34\x6a\x00\xff\xd2\x8b\x4c\x24\x20\x89\x0d\x00\x00\x00\x00\x85\xc9\x74\x00\x8b\x01\x8b\x80\x80\x00\x00\x00\x8d\x54\x24\x04\x52\xff\xd0\x8b\x0d\x00\x00\x00\x00\x8b\x11\x8b\x92\x84\x00\x00\x00\x8d\x44\x24\x10\x50\xff\xd2";

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
	DWORD address = FindPattern(FixOffset(0x500000), 0x100000,
		ZoomCameraMaxDistance_Pattern, ZoomCameraMaxDistance_Mask);
	ZoomCameraMaxDistance = GetDataPtrAtOffset<float>(address, ZoomCameraMaxDistance_Offset);

	if (!ZoomCameraMaxDistance) {
		WriteChatf(PLUGIN_MSG "\arFailed to find [ZoomCameraMaxDistance]\ax");
		return false;
	}

	address = FindPattern(FixOffset(0x700000), 0x100000,
		UserCameraMaxDistance_Pattern, UserCameraMaxDistance_Mask);
	UserCameraMaxDistance = GetDataPtrAtOffset<float>(address, UserCameraMaxDistance_Offset);

	if (!UserCameraMaxDistance) {
		WriteChatf(PLUGIN_MSG "\arFailed to find [UserCameraMaxDistance]\ax");
		return false;
	}

	if ((CDisplay__SetViewActor_Offset = FindPattern(FixOffset(0x400000), 0x100000,
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
