#pragma once
#include <ntddk.h>
#include <wdm.h>



#ifndef __HOOK__
#define __HOOK__

	void testHook();
	void unhook();
	void setProcessId(unsigned int pid);

#endif


