#include "Driver.h"
#include "Driver.tmh"
#include "Device.h"

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)

NTSTATUS DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

#if UMDF_VERSION_MAJOR == 2 && UMDF_VERSION_MINOR == 0
	WPP_INIT_TRACING(NfcMFRC522TraceGuid);
#else
	WPP_INIT_TRACING(driverObject, registryPath);
#endif

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

	WDF_DRIVER_CONFIG config;
	WDF_DRIVER_CONFIG_INIT(&config, DeviceAdd);

	config.EvtDriverUnload = DriverUnload;

	if (!NT_SUCCESS(status = WdfDriverCreate(driverObject, registryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!", status);
#if UMDF_VERSION_MAJOR == 2 && UMDF_VERSION_MINOR == 0
		WPP_CLEANUP();
#else
		WPP_CLEANUP(driverObject);
#endif
		return status;
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

	return status;
}

void DriverUnload(WDFDRIVER wdfDriver)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

	UNREFERENCED_PARAMETER(wdfDriver);

#if UMDF_VERSION_MAJOR == 2 && UMDF_VERSION_MINOR == 0
	WPP_CLEANUP();
#else
	WPP_CLEANUP(WdfDriverWdmGetDriverObject(wdfDriver));
#endif
}