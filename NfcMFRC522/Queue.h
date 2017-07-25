#pragma once
#include "Driver.h"

EXTERN_C_START

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL QueueIoDeviceControl;

EXTERN_C_END

class Device;

class Queue
{
	friend class Device;
	friend EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL QueueIoDeviceControl;

public:
	Queue(_In_ WDFQUEUE wdfQueue);
	~Queue();

	Device* GetDevice() const;

private:
	NTSTATUS Initialize();
	NTSTATUS Deinitialize();
	//NTSTATUS OnFileCreate(_In_ WDFREQUEST wdfRequest, _In_ WDFFILEOBJECT wdfFileObject);
	//NTSTATUS OnFileClose(_In_ WDFFILEOBJECT wdfFileObject);
	NTSTATUS OnIoDeviceControl(_In_ WDFREQUEST wdfRequest, _In_ ULONG ioControlCode, _In_ size_t inputBufferLength, _In_ size_t outputBufferLength);

	WDFQUEUE m_queue;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(Queue, GetQueueObject);