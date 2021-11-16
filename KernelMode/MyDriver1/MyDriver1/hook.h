#pragma once
#include <ntifs.h>
#include <ntddk.h>
#include <wdm.h>

#ifndef __HOOK__
#define __HOOK__

typedef PVOID(*PsGetProcessDebugPort_t)(
	IN	PEPROCESS Process
	);

NTSTATUS testHook();
void unhook();
void setProcessId(unsigned int pid);
PsGetProcessDebugPort_t PsGetProcessDebugPort;

#endif