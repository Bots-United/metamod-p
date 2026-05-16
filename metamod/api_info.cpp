// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// api_info.cpp - info for api routines

/*
 * Copyright (c) 2001-2006 Will Day <willday@hpgx.net>
 *
 *    This file is part of Metamod.
 *
 *    Metamod is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Metamod is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Metamod; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#include <extdll.h>			// always

#include "api_info.h"		// me
#include "api_hook.h"

// loglevel, api_caller, name
const dllapi_info_t dllapi_info = {
	{3,	api_caller_void_args_void, 	"GameDLLInit" },		// pfnGameInit
	{10,	api_caller_int_args_p, 		"DispatchSpawn" },		// pfnSpawn
	{16,	api_caller_void_args_p,		"DispatchThink" },		// pfnThink
	{9,	api_caller_void_args_2p,	"DispatchUse" },		// pfnUse
	{11,	api_caller_void_args_2p,	"DispatchTouch" },		// pfnTouch
	{9,	api_caller_void_args_2p,	"DispatchBlocked" },		// pfnBlocked
	{10,	api_caller_void_args_2p,	"DispatchKeyValue" },		// pfnKeyValue
	{9,	api_caller_void_args_2p,	"DispatchSave" },		// pfnSave
	{9,	api_caller_int_args_2pi,	"DispatchRestore" },		// pfnRestore
	{20,	api_caller_void_args_p,		"DispatchObjectCollsionBox" },	// pfnSetAbsBox
	{9,	api_caller_void_args_4pi,	"SaveWriteFields" },		// pfnSaveWriteFields
	{9,	api_caller_void_args_4pi,	"SaveReadFields" },		// pfnSaveReadFields
	{9,	api_caller_void_args_p,		"SaveGlobalState" },		// pfnSaveGlobalState
	{9,	api_caller_void_args_p,		"RestoreGlobalState" },		// pfnRestoreGlobalState
	{9,	api_caller_void_args_void, 	"ResetGlobalState" },	// pfnResetGlobalState
	{3,	api_caller_int_args_4p, 	"ClientConnect" },		// pfnClientConnect
	{3,	api_caller_void_args_p,		"ClientDisconnect" },	// pfnClientDisconnect
	{3,	api_caller_void_args_p,		"ClientKill" },			// pfnClientKill
	{3,	api_caller_void_args_p,		"ClientPutInServer" },	// pfnClientPutInServer
	{9,	api_caller_void_args_p,		"ClientCommand" },		// pfnClientCommand
	{11,	api_caller_void_args_2p,	"ClientUserInfoChanged" },	// pfnClientUserInfoChanged
	{3,	api_caller_void_args_p2i,	"ServerActivate" },		// pfnServerActivate
	{3,	api_caller_void_args_void,	"ServerDeactivate" },	// pfnServerDeactivate
	{14,	api_caller_void_args_p,		"PlayerPreThink" },		// pfnPlayerPreThink
	{14,	api_caller_void_args_p,		"PlayerPostThink" },	// pfnPlayerPostThink
	{18,	api_caller_void_args_void,	"StartFrame" },			// pfnStartFrame
	{9,	api_caller_void_args_void,	"ParmsNewLevel" },		// pfnParmsNewLevel
	{9,	api_caller_void_args_void,	"ParmsChangeLevel" },	// pfnParmsChangeLevel
	{9,	api_caller_ptr_args_void,	"GetGameDescription" },	// pfnGetGameDescription
	{9,	api_caller_void_args_2p,	"PlayerCustomization" },	// pfnPlayerCustomization
	{9,	api_caller_void_args_p,		"SpectatorConnect" },	// pfnSpectatorConnect
	{9,	api_caller_void_args_p,		"SpectatorDisconnect" },	// pfnSpectatorDisconnect
	{9,	api_caller_void_args_p,		"SpectatorThink" },		// pfnSpectatorThink
	{3,	api_caller_void_args_p,		"Sys_Error" },			// pfnSys_Error
	{13,	api_caller_void_args_pi,	"PM_Move" },			// pfnPM_Move
	{9,	api_caller_void_args_p,		"PM_Init" },			// pfnPM_Init
	{9,	api_caller_char_args_p,		"PM_FindTextureType" },	// pfnPM_FindTextureType
	{12,	api_caller_void_args_4p,	"SetupVisibility" },	// pfnSetupVisibility
	{12,	api_caller_void_args_pip,	"UpdateClientData" },	// pfnUpdateClientData
	{16,	api_caller_int_args_pi2p2ip,	"AddToFullPack" },		// pfnAddToFullPack
	{9,	api_caller_void_args_2i2pi2p,	"CreateBaseline" },		// pfnCreateBaseline
	{9,	api_caller_void_args_void,	"RegisterEncoders" },	// pfnRegisterEncoders
	{9,	api_caller_int_args_2p,		"GetWeaponData" },		// pfnGetWeaponData
	{15,	api_caller_void_args_2pui,	"CmdStart" },			// pfnCmdStart
	{15,	api_caller_void_args_p,		"CmdEnd" },				// pfnCmdEnd
	{9,	api_caller_int_args_4p,		"ConnectionlessPacket" },	// pfnConnectionlessPacket
	{9,	api_caller_int_args_i2p,	"GetHullBounds" },		// pfnGetHullBounds
	{9,	api_caller_void_args_void,	"CreateInstancedBaselines" },	// pfnCreateInstancedBaselines
	{3,	api_caller_int_args_3p,		"InconsistentFile" },	// pfnInconsistentFile
	{20,	api_caller_int_args_void,	"AllowLagCompensation" },	// pfnAllowLagCompensation
	{0,	NULL, 	NULL },
};

const newapi_info_t newapi_info = {
	{16,	api_caller_void_args_p,		"OnFreeEntPrivateData" },	// pfnOnFreeEntPrivateData
	{3,	api_caller_void_args_void,	"GameShutdown" },			// pfnGameShutdown
	{14,	api_caller_int_args_2p,		"ShouldCollide" },			// pfnShouldCollide
	// Added 2005/08/11 (no SDK update):
	{3,	api_caller_void_args_2p,	"CvarValue" },			// pfnCvarValue
	// Added 2005/11/21 (no SDK update):
	{3,	api_caller_void_args_pi2p,	"CvarValue2" },			// pfnCvarValue2
	{0,	NULL, 	NULL },
};

const engine_info_t engine_info = {
	{13,	api_caller_int_args_p,		"PrecacheModel" },		// pfnPrecacheModel
	{13,	api_caller_int_args_p,		"PrecacheSound" },		// pfnPrecacheSound
	{18,	api_caller_void_args_2p,	"SetModel" },			// pfnSetModel
	{34,	api_caller_int_args_p,		"ModelIndex" },			// pfnModelIndex
	{10,	api_caller_int_args_i,		"ModelFrames" },		// pfnModelFrames
	{14,	api_caller_void_args_3p,	"SetSize" },			// pfnSetSize
	{9,	api_caller_void_args_2p,	"ChangeLevel" },		// pfnChangeLevel
	{9,	api_caller_void_args_p,		"GetSpawnParms" },		// pfnGetSpawnParms
	{9,	api_caller_void_args_p,		"SaveSpawnParms" },		// pfnSaveSpawnParms
	{9,	api_caller_float_args_p,	"VecToYaw" },			// pfnVecToYaw
	{14,	api_caller_void_args_2p,	"VecToAngles" },		// pfnVecToAngles
	{9,	api_caller_void_args_2pfi,	"MoveToOrigin" },		// pfnMoveToOrigin
	{9,	api_caller_void_args_p,		"ChangeYaw" },			// pfnChangeYaw
	{9,	api_caller_void_args_p,		"ChangePitch" },		// pfnChangePitch
	{32,	api_caller_ptr_args_3p,		"FindEntityByString" },		// pfnFindEntityByString
	{9,	api_caller_int_args_p,		"GetEntityIllum" },		// pfnGetEntityIllum
	{9,	api_caller_ptr_args_2pf,	"FindEntityInSphere" },		// pfnFindEntityInSphere
	{19,	api_caller_ptr_args_p,		"FindClientInPVS" },		// pfnFindClientInPVS
	{9,	api_caller_ptr_args_p,		"EntitiesInPVS" },		// pfnEntitiesInPVS
	{40,	api_caller_void_args_p,		"MakeVectors" },		// pfnMakeVectors
	{9,	api_caller_void_args_4p,	"AngleVectors" },		// pfnAngleVectors
	{13,	api_caller_ptr_args_void,	"CreateEntity" },		// pfnCreateEntity
	{13,	api_caller_void_args_p,		"RemoveEntity" },		// pfnRemoveEntity
	{13,	api_caller_ptr_args_i,		"CreateNamedEntity" },		// pfnCreateNamedEntity
	{9,	api_caller_void_args_p,		"MakeStatic" },			// pfnMakeStatic
	{9,	api_caller_int_args_p,		"EntIsOnFloor" },		// pfnEntIsOnFloor
	{9,	api_caller_int_args_p,		"DropToFloor" },		// pfnDropToFloor
	{9,	api_caller_int_args_p2fi,	"WalkMove" },			// pfnWalkMove
	{14,	api_caller_void_args_2p,	"SetOrigin" },			// pfnSetOrigin
	{12,	api_caller_void_args_pip2f2i,	"EmitSound" },			// pfnEmitSound
	{12,	api_caller_void_args_3p2f2i,	"EmitAmbientSound" },		// pfnEmitAmbientSound
	{20,	api_caller_void_args_2pi2p,	"TraceLine" },			// pfnTraceLine
	{9,	api_caller_void_args_3p,	"TraceToss" },			// pfnTraceToss
	{9,	api_caller_int_args_3pi2p,	"TraceMonsterHull" },		// pfnTraceMonsterHull
	{9,	api_caller_void_args_2p2i2p,	"TraceHull" },			// pfnTraceHull
	{9,	api_caller_void_args_2pi2p,	"TraceModel" },			// pfnTraceModel
	{15,	api_caller_ptr_args_3p,		"TraceTexture" },		// pfnTraceTexture		// CS: when moving
	{9,	api_caller_void_args_2pif2p,	"TraceSphere" },		// pfnTraceSphere
	{9,	api_caller_void_args_pfp,	"GetAimVector" },		// pfnGetAimVector
	{9,	api_caller_void_args_p,		"ServerCommand" },		// pfnServerCommand
	{9,	api_caller_void_args_void,	"ServerExecute" },		// pfnServerExecute
	{11,	api_caller_void_args_2pV,	"engClientCommand" },		// pfnClientCommand		// d'oh, ClientCommand in dllapi too
	{9,	api_caller_void_args_2p2f,	"ParticleEffect" },		// pfnParticleEffect
	{9,	api_caller_void_args_ip,	"LightStyle" },			// pfnLightStyle
	{9,	api_caller_int_args_p,		"DecalIndex" },			// pfnDecalIndex
	{15,	api_caller_int_args_p,		"PointContents" },		// pfnPointContents		// CS: when moving
	{22,	api_caller_void_args_2i2p,	"MessageBegin" },		// pfnMessageBegin
	{22,	api_caller_void_args_void,	"MessageEnd" },			// pfnMessageEnd
	{30,	api_caller_void_args_i,		"WriteByte" },			// pfnWriteByte
	{23,	api_caller_void_args_i,		"WriteChar" },			// pfnWriteChar
	{24,	api_caller_void_args_i,		"WriteShort" },			// pfnWriteShort
	{23,	api_caller_void_args_i,		"WriteLong" },			// pfnWriteLong
	{23,	api_caller_void_args_f,		"WriteAngle" },			// pfnWriteAngle
	{23,	api_caller_void_args_f,		"WriteCoord" },			// pfnWriteCoord
	{25,	api_caller_void_args_p,		"WriteString" },		// pfnWriteString
	{23,	api_caller_void_args_i,		"WriteEntity" },		// pfnWriteEntity
	{9,	api_caller_void_args_p,		"CVarRegister" },		// pfnCVarRegister
	{21,	api_caller_float_args_p,	"CVarGetFloat" },		// pfnCVarGetFloat
	{9,	api_caller_ptr_args_p,		"CVarGetString" },		// pfnCVarGetString
	{10,	api_caller_void_args_pf,	"CVarSetFloat" },		// pfnCVarSetFloat
	{9,	api_caller_void_args_2p,	"CVarSetString" },		// pfnCVarSetString
	{15,	api_caller_void_args_ipV,	"AlertMessage" },		// pfnAlertMessage
	{17,	api_caller_void_args_2pV,	"EngineFprintf" },		// pfnEngineFprintf
	{14,	api_caller_ptr_args_pi,		"PvAllocEntPrivateData" },	// pfnPvAllocEntPrivateData
	{9,	api_caller_ptr_args_p,		"PvEntPrivateData" },		// pfnPvEntPrivateData
	{9,	api_caller_void_args_p,		"FreeEntPrivateData" },		// pfnFreeEntPrivateData
	{9,	api_caller_ptr_args_i,		"SzFromIndex" },		// pfnSzFromIndex
	{10,	api_caller_int_args_p,		"AllocString" },		// pfnAllocString
	{9,	api_caller_ptr_args_p,		"GetVarsOfEnt" },		// pfnGetVarsOfEnt
	{14,	api_caller_ptr_args_i,		"PEntityOfEntOffset" },		// pfnPEntityOfEntOffset
	{19,	api_caller_int_args_p,		"EntOffsetOfPEntity" },		// pfnEntOffsetOfPEntity
	{14,	api_caller_int_args_p,		"IndexOfEdict" },		// pfnIndexOfEdict
	{17,	api_caller_ptr_args_i,		"PEntityOfEntIndex" },		// pfnPEntityOfEntIndex
	{9,	api_caller_ptr_args_p,		"FindEntityByVars" },		// pfnFindEntityByVars
	{14,	api_caller_ptr_args_p,		"GetModelPtr" },		// pfnGetModelPtr
	{9,	api_caller_int_args_pi,		"RegUserMsg" },			// pfnRegUserMsg
	{9,	api_caller_void_args_pf,	"AnimationAutomove" },		// pfnAnimationAutomove
	{9,	api_caller_void_args_pi2p,	"GetBonePosition" },		// pfnGetBonePosition
	{9,	api_caller_uint_args_p,		"FunctionFromName" },		// pfnFunctionFromName
	{9,	api_caller_ptr_args_ui,		"NameForFunction" },		// pfnNameForFunction
	{9,	api_caller_void_args_pip,	"ClientPrintf" },		// pfnClientPrintf
	{9,	api_caller_void_args_p,		"ServerPrint" },		// pfnServerPrint
	{13,	api_caller_ptr_args_void,	"Cmd_Args" },			// pfnCmd_Args
	{13,	api_caller_ptr_args_i,		"Cmd_Argv" },			// pfnCmd_Argv
	{13,	api_caller_int_args_void,	"Cmd_Argc" },			// pfnCmd_Argc
	{9,	api_caller_void_args_pi2p,	"GetAttachment" },		// pfnGetAttachment
	{9,	api_caller_void_args_p,		"CRC32_Init" },			// pfnCRC32_Init
	{9,	api_caller_void_args_2pi,	"CRC32_ProcessBuffer" },	// pfnCRC32_ProcessBuffer
	{9,	api_caller_void_args_puc,	"CRC32_ProcessByte" },		// pfnCRC32_ProcessByte
	{9,	api_caller_ulong_args_ul,	"CRC32_Final" },		// pfnCRC32_Final
	{16,	api_caller_int_args_2i,		"RandomLong" },			// pfnRandomLong
	{14,	api_caller_float_args_2f,	"RandomFloat" },		// pfnRandomFloat		// CS: when firing
	{14,	api_caller_void_args_2p,	"SetView" },			// pfnSetView
	{9,	api_caller_float_args_void,	"Time" },			// pfnTime
	{9,	api_caller_void_args_p2f,	"CrosshairAngle" },		// pfnCrosshairAngle
	{10,	api_caller_ptr_args_2p,		"LoadFileForMe" },		// pfnLoadFileForMe
	{10,	api_caller_void_args_p,		"FreeFile" },			// pfnFreeFile
	{9,	api_caller_void_args_p,		"EndSection" },			// pfnEndSection
	{9,	api_caller_int_args_3p,		"CompareFileTime" },		// pfnCompareFileTime
	{9,	api_caller_void_args_p,		"GetGameDir" },			// pfnGetGameDir
	{9,	api_caller_void_args_p,		"Cvar_RegisterVariable" },	// pfnCvar_RegisterVariable
	{9,	api_caller_void_args_p4i,	"FadeClientVolume" },		// pfnFadeClientVolume
	{14,	api_caller_void_args_pf,	"SetClientMaxspeed" },		// pfnSetClientMaxspeed
	{9,	api_caller_ptr_args_p,		"CreateFakeClient" },		// pfnCreateFakeClient
	{9,	api_caller_void_args_2p3fus2uc,	"RunPlayerMove" },		// pfnRunPlayerMove
	{9,	api_caller_int_args_void,	"NumberOfEntities" },		// pfnNumberOfEntities
	{17,	api_caller_ptr_args_p,		"GetInfoKeyBuffer" },		// pfnGetInfoKeyBuffer
	{13,	api_caller_ptr_args_2p,		"InfoKeyValue" },		// pfnInfoKeyValue
	{9,	api_caller_void_args_3p,	"SetKeyValue" },		// pfnSetKeyValue
	{12,	api_caller_void_args_i3p,	"SetClientKeyValue" },		// pfnSetClientKeyValue
	{9,	api_caller_int_args_p,		"IsMapValid" },			// pfnIsMapValid
	{9,	api_caller_void_args_p3i,	"StaticDecal" },		// pfnStaticDecal
	{9,	api_caller_int_args_p,		"PrecacheGeneric" },		// pfnPrecacheGeneric
	{10,	api_caller_int_args_p,		"GetPlayerUserId" },		// pfnGetPlayerUserId
	{9,	api_caller_void_args_pip2f4i2p,	"BuildSoundMsg" },		// pfnBuildSoundMsg
	{9,	api_caller_int_args_void,	"IsDedicatedServer" },		// pfnIsDedicatedServer
	{9,	api_caller_ptr_args_p,		"CVarGetPointer" },		// pfnCVarGetPointer
	{9,	api_caller_uint_args_p,		"GetPlayerWONId" },		// pfnGetPlayerWONId
	{9,	api_caller_void_args_2p,	"Info_RemoveKey" },		// pfnInfo_RemoveKey
	{15,	api_caller_ptr_args_2p,		"GetPhysicsKeyValue" },		// pfnGetPhysicsKeyValue
	{14,	api_caller_void_args_3p,	"SetPhysicsKeyValue" },		// pfnSetPhysicsKeyValue
	{15,	api_caller_ptr_args_p,		"GetPhysicsInfoString" },	// pfnGetPhysicsInfoString
	{13,	api_caller_ushort_args_ip,	"PrecacheEvent" },		// pfnPrecacheEvent
	{9,	api_caller_void_args_ipusf2p2f4i,"PlaybackEvent" },		// pfnPlaybackEvent
	{31,	api_caller_ptr_args_p,		"SetFatPVS" },			// pfnSetFatPVS
	{31,	api_caller_ptr_args_p,		"SetFatPAS" },			// pfnSetFatPAS
	{50,	api_caller_int_args_2p,		"CheckVisibility" },		// pfnCheckVisibility
	{37,	api_caller_void_args_2p,	"DeltaSetField" },		// pfnDeltaSetField
	{38,	api_caller_void_args_2p,	"DeltaUnsetField" },		// pfnDeltaUnsetField
	{9,	api_caller_void_args_2p,	"DeltaAddEncoder" },		// pfnDeltaAddEncoder
	{45,	api_caller_int_args_void,	"GetCurrentPlayer" },		// pfnGetCurrentPlayer
	{14,	api_caller_int_args_p,		"CanSkipPlayer" },		// pfnCanSkipPlayer
	{9,	api_caller_int_args_2p,		"DeltaFindField" },		// pfnDeltaFindField
	{37,	api_caller_void_args_pi,	"DeltaSetFieldByIndex" },	// pfnDeltaSetFieldByIndex
	{38,	api_caller_void_args_pi,	"DeltaUnsetFieldByIndex" },	// pfnDeltaUnsetFieldByIndex
	{9,	api_caller_void_args_2i,	"SetGroupMask" },		// pfnSetGroupMask
	{9,	api_caller_int_args_ip,		"engCreateInstancedBaseline" },	// pfnCreateInstancedBaseline		// d'oh, CreateInstancedBaseline in dllapi too
	{9,	api_caller_void_args_2p,	"Cvar_DirectSet" },		// pfnCvar_DirectSet
	{9,	api_caller_void_args_i3p,	"ForceUnmodified" },		// pfnForceUnmodified
	{9,	api_caller_void_args_3p,	"GetPlayerStats" },		// pfnGetPlayerStats
	{3,	api_caller_void_args_2p,	"AddServerCommand" },		// pfnAddServerCommand
	// Added in SDK 2.2:
	{9,	api_caller_int_args_2i,		"Voice_GetClientListening" },	// Voice_GetClientListening
	{9,	api_caller_int_args_3i,		"Voice_SetClientListening" },	// Voice_SetClientListening
	// Added for HL 1109 (no SDK update):
	{9,	api_caller_ptr_args_p,		"GetPlayerAuthId" },		// pfnGetPlayerAuthId
	// Added 2003/11/10 (no SDK update):
	{30,	api_caller_ptr_args_2p,		"SequenceGet" },		// pfnSequenceGet
	{30,	api_caller_ptr_args_pip,	"SequencePickSentence" },	// pfnSequencePickSentence
	{30,	api_caller_int_args_p,		"GetFileSize" },		// pfnGetFileSize
	{30,	api_caller_uint_args_p,		"GetApproxWavePlayLen" },	// pfnGetApproxWavePlayLen
	{30,	api_caller_int_args_void,	"IsCareerMatch" },		// pfnIsCareerMatch
	{30,	api_caller_int_args_p,		"GetLocalizedStringLength" },	// pfnGetLocalizedStringLength
	{30,	api_caller_void_args_i,		"RegisterTutorMessageShown" },	// pfnRegisterTutorMessageShown
	{30,	api_caller_int_args_i,		"GetTimesTutorMessageShown" },	// pfnGetTimesTutorMessageShown
	{30,	api_caller_void_args_pi,	"ProcessTutorMessageDecayBuffer" },	// pfnProcessTutorMessageDecayBuffer
	{30,	api_caller_void_args_pi,	"ConstructTutorMessageDecayBuffer" },	// pfnConstructTutorMessageDecayBuffer
	{9,	api_caller_void_args_void,	"ResetTutorMessageDecayData" },	// pfnResetTutorMessageDecayData
	// Added 2005/08/11 (no SDK update):
	{3,	api_caller_void_args_2p,	"QueryClientCvarValue" },	// pfnQueryClientCvarValue
	// Added 2005/11/21 (no SDK update):
	{3,	api_caller_void_args_2pi,	"QueryClientCvarValue2" },	// pfnQueryClientCvarValue2
	// Added 2009-06-17 (no SDK update):
	{8,	api_caller_int_args_2p,		"EngCheckParm" },		// pfnEngCheckParm
	// Added 2024/08/21 (HL25 SDK update):
	{26,	api_caller_ptr_args_i,		"PEntityOfEntIndexAllEntities" },	// pfnPEntityOfEntIndexAllEntities
	// end
	{0,   NULL,	NULL },
};
