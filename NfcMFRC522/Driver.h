#pragma once
#define INITGUID

#include <windows.h>
#include <wdf.h>
#include <nfccx.h>
#include <winsmcrd.h>
#include <initguid.h>
#include <ntassert.h>
#include <ntstatus.h>
#include <winsmcrd.h>
#include <stdlib.h>
#include <strsafe.h>
#include <new.h>
#include <objbase.h>
#include <coguid.h>
#include <nfpdev.h>
#include <nfcsedev.h>
#include <nfcradiodev.h>
#include <ncidef.h>
#include <reshub.h>
#include <gpio.h>
#include <spb.h>
#include <devioctl.h>
#include "Trace.h"

#define ReturnIfFailed(status, operation) if (!NT_SUCCESS(status = operation)) \
											return status

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD DriverUnload;

EXTERN_C_END
