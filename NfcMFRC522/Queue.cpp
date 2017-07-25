#include "Queue.h"
#include "Queue.tmh"
#include "Constants.h"
#include "Device.h"
#include "FileObject.h"

#pragma alloc_text(PAGE, QueueIoDeviceControl)

void QueueIoDeviceControl(WDFQUEUE wdfQueue, WDFREQUEST wdfRequest, size_t outputBufferLength, size_t inputBufferLength, ULONG ioControlCode)
{
	PAGED_CODE();

	Queue* queue = GetQueueObject(wdfQueue);

	NT_ASSERT(queue != nullptr);

	queue->OnIoDeviceControl(wdfRequest, ioControlCode, inputBufferLength, outputBufferLength);
}


Queue::Queue(WDFQUEUE wdfQueue) :
	m_queue(wdfQueue)
{
	NT_ASSERT(m_queue != nullptr);
}

Queue::~Queue()
{
	m_queue = nullptr;
}

NTSTATUS Queue::Initialize()
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	DECLARE_CONST_UNICODE_STRING(RMNamespace, RM_NAMESPACE);
	DECLARE_CONST_UNICODE_STRING(SCNamespace, SMARTCARD_READER_NAMESPACE);

	auto wdfDevice = WdfIoQueueGetDevice(m_queue);
	if (!NT_SUCCESS(status = WdfDeviceCreateDeviceInterface(wdfDevice, &GUID_NFC_RADIO_MEDIA_DEVICE_INTERFACE, &RMNamespace)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfDeviceCreateDeviceInterface failed %!STATUS!", status);
		return status;
	}

	if (!NT_SUCCESS(status = WdfDeviceCreateDeviceInterface(wdfDevice, &GUID_DEVINTERFACE_NFP, nullptr)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfDeviceCreateDeviceInterface failed %!STATUS!", status);
		return status;
	}

	if (!NT_SUCCESS(status = WdfDeviceCreateDeviceInterface(wdfDevice, &GUID_DEVINTERFACE_SMARTCARD_READER, &SCNamespace)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfDeviceCreateDeviceInterface failed %!STATUS!", status);
		return status;
	}

	if (!NT_SUCCESS(status = WdfDeviceCreateDeviceInterface(wdfDevice, &GUID_DEVINTERFACE_NFCSE, nullptr)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfDeviceCreateDeviceInterface failed %!STATUS!", status);
		return status;
	}

	return status;
}

NTSTATUS Queue::Deinitialize()
{
	return STATUS_SUCCESS;
}

Device* Queue::GetDevice() const
{
	return GetDeviceObject(WdfIoQueueGetDevice(m_queue));
}

//NTSTATUS Queue::OnFileCreate(WDFREQUEST wdfRequest, WDFFILEOBJECT wdfFileObject)
//{
//	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Entry");
//
//	UNREFERENCED_PARAMETER(wdfRequest);
//
//	NTSTATUS status = STATUS_SUCCESS;
//
//	auto fileObject = new (GetFileObject(wdfFileObject)) FileObject(wdfFileObject);
//	if (fileObject == nullptr)
//	{
//		status = STATUS_INSUFFICIENT_RESOURCES;
//		TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "Queue::OnFileCreate failed %!STATUS!", status);
//
//		WdfRequestComplete(wdfRequest, status);
//		return status;
//	}
//
//	auto fileName = WdfFileObjectGetFileName(wdfFileObject);
//	if (fileName != nullptr && fileName->Length > 0)
//	{
//		NT_ASSERT(fileName->Buffer != nullptr);
//	}
//
//	WdfRequestComplete(wdfRequest, STATUS_SUCCESS);
//	return status;
//}
//
//NTSTATUS Queue::OnFileClose(WDFFILEOBJECT wdfFileObject)
//{
//	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Entry");
//
//	FileObject* fileObject = GetFileObject(wdfFileObject);
//
//	NT_ASSERT(fileObject != nullptr);
//
//	return STATUS_SUCCESS;
//}

NTSTATUS Queue::OnIoDeviceControl(WDFREQUEST wdfRequest, ULONG ioControlCode, size_t inputBufferLength, size_t outputBufferLength)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Entry");

	UNREFERENCED_PARAMETER(wdfRequest);
	UNREFERENCED_PARAMETER(inputBufferLength);
	UNREFERENCED_PARAMETER(outputBufferLength);

	NTSTATUS status = STATUS_INVALID_DEVICE_STATE;

	switch (ioControlCode)
	{
	case IOCTL_NFP_GET_MAX_MESSAGE_BYTES:
	case IOCTL_NFP_GET_KILO_BYTES_PER_SECOND:
	case IOCTL_NFP_DISABLE:
	case IOCTL_NFP_ENABLE:
	case IOCTL_NFP_SET_PAYLOAD:
	case IOCTL_NFP_GET_NEXT_SUBSCRIBED_MESSAGE:
	case IOCTL_NFP_GET_NEXT_TRANSMITTED_MESSAGE:

	case IOCTL_NFCSE_ENUM_ENDPOINTS:
	case IOCTL_NFCSE_SUBSCRIBE_FOR_EVENT:
	case IOCTL_NFCSE_GET_NEXT_EVENT:
	case IOCTL_NFCSE_SET_CARD_EMULATION_MODE:
	case IOCTL_NFCSE_GET_NFCC_CAPABILITIES:
	case IOCTL_NFCSE_GET_ROUTING_TABLE:
	case IOCTL_NFCSE_SET_ROUTING_TABLE:
	case IOCTL_NFCSE_HCE_REMOTE_RECV:
	case IOCTL_NFCSE_HCE_REMOTE_SEND:

	case IOCTL_SMARTCARD_EJECT:
	case IOCTL_SMARTCARD_GET_LAST_ERROR:
	case IOCTL_SMARTCARD_SET_ATTRIBUTE:
	case IOCTL_SMARTCARD_SWALLOW:
	case IOCTL_SMARTCARD_GET_ATTRIBUTE:
	case IOCTL_SMARTCARD_GET_STATE:
	case IOCTL_SMARTCARD_IS_ABSENT:
	case IOCTL_SMARTCARD_IS_PRESENT:
	case IOCTL_SMARTCARD_POWER:
	case IOCTL_SMARTCARD_SET_PROTOCOL:
	case IOCTL_SMARTCARD_TRANSMIT:

	case IOCTL_NFCRM_SET_RADIO_STATE:
	case IOCTL_NFCRM_QUERY_RADIO_STATE:
	case IOCTL_NFCSERM_SET_RADIO_STATE:
	case IOCTL_NFCSERM_QUERY_RADIO_STATE:

	break;
	}

	if (status != STATUS_PENDING)
	{
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "%!FUNC! Completing Request with Status %!STATUS!", status);
		WdfRequestComplete(wdfRequest, status);
	}

	return status;
}