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

	//.text:00553F32 loc_553F32:                             ; CODE XREF: CEverQuest::MouseWheelScrolled(int)+50j
	//.text:00553F32                 mov     eax, g_pZoomCamera
	//.text:00553F37                 fld     dword ptr [eax+34h]
	//.text:00553F3A                 fstp    [esp+20h+var_1C]
	//.text:00553F3E                 fld     ds:flt_ABDB7C
	//.text:00553F44                 fstp    [esp+20h+var_18]
	//.text:00553F48                 fld     ds:float const g_fMaxZoomCameraDistance <--
	//.text:00553F4E                 fstp    [esp+20h+var_14]
	//.text:00553F52                 call    sub_5F2780
	//.text:00553F57                 mov     eax, [eax+90h]
	//.text:00553F5D                 push    eax
	//.text:00553F5E                 call    sub_88D6E0

	// 0xACE2B4 12-09-2015
	const char* ZoomCameraMaxDistance_Mask = "x????xxxxxxxxx????xxxxxx????xxxxx????xxxxxxxx????";
	const unsigned char* ZoomCameraMaxDistance_Pattern = (const unsigned char*)"\xa1\x00\x00\x00\x00\xd9\x40\x34\xd9\x5c\x24\x04\xd9\x05\x00\x00\x00\x00\xd9\x5c\x24\x08\xd9\x05\x00\x00\x00\x00\xd9\x5c\x24\x0c\xe8\x00\x00\x00\x00\x8b\x80\x90\x00\x00\x00\x50\xe8\x00\x00\x00\x00";
	const int ZoomCameraMaxDistance_Offset = (0x553F48 + 2) - 0x553F32;
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
}

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
	sscanf(szValue, "%f", &fValue);

	return fValue;
}

VOID Cmd_Camera(PSPAWNINFO pChar, PCHAR szLine)
{
	if (!s_initialized) {
		WriteChatf(PLUGIN_MSG "Plugin is disabled due to initialization error.");
		return;
	}

	CHAR Command[MAX_STRING];
	GetArg(Command, szLine, 1);

	CHAR Param[MAX_STRING];
	GetArg(Param, szLine, 2);

	if (!stricmp(Command, "distance")) {
		bool reset = !stricmp(Command, "reset");
		if (reset)
			ResetCameraDistance();
		else
			SetCameraDistance(static_cast<float>(atof(Param)));

		// check to see if we want to save
		GetArg(Command, szLine, 3);
		if (!stricmp(Command, "save")) {
			SaveCameraDistance(reset ? 0.0f : static_cast<float>(atof(Param)));
		}
	}
	else if (!stricmp(Command, "info")) {
		WriteChatf(PLUGIN_MSG "Zoom camera max distance: \ay%.2f\ax (default: %.2f)",
			*ZoomCameraMaxDistance, s_origZoomCameraMaxDistance);
		WriteChatf(PLUGIN_MSG "User camera max distance: \ay%.2f\ax (default: %.2f)",
			*UserCameraMaxDistance, s_origUserCameraMaxDistance);
	}
	else {
		WriteChatf(PLUGIN_MSG "Usage:");
		WriteChatf(PLUGIN_MSG "\ag/camera distance [ reset | <distance> ] [ save ]\ax - set camera max distance or reset to default. Specify 'save' to save the value");
		WriteChatf(PLUGIN_MSG "\ag/camera info\ax - report current camera information");
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
