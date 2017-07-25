#include "Device.h"
#include "Device.tmh"
#include "Queue.h"
#include "FileObject.h"

#pragma alloc_text(PAGE, DeviceAdd)
#pragma alloc_text(PAGE, DeviceCleanup)
#pragma alloc_text(PAGE, DeviceD0Entry)
#pragma alloc_text(PAGE, DeviceD0Exit)
#pragma alloc_text(PAGE, DevicePrepareHardware)
#pragma alloc_text(PAGE, DeviceReleaseHardware)
#pragma alloc_text(PAGE, DeviceInterruptIsr)
#pragma alloc_text(PAGE, DeviceInterruptDpc)
#pragma alloc_text(PAGE, DeviceFileCreate)
#pragma alloc_text(PAGE, DeviceFileClose)

#define DEVICE_RESOURCE_SERIAL 0x1
#define DEVICE_RESOURCE_GPIO 0x2
#define DEVICE_RESOURCE_INTERRUPT 0x4
#define DEVICE_RESOURCES (DEVICE_RESOURCE_SERIAL | DEVICE_RESOURCE_GPIO | DEVICE_RESOURCE_INTERRUPT)
#define FIFO_MAX_LENGTH 64
#define UID_MAX_LENGTH 10
#define STATUS_CARD_COLLISION ((NTSTATUS)0x2000000CL)

NTSTATUS DeviceAdd(WDFDRIVER wdfDriver, PWDFDEVICE_INIT deviceInit)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	UNREFERENCED_PARAMETER(wdfDriver);

	PAGED_CODE();

	NTSTATUS status = STATUS_SUCCESS;

	WdfDeviceInitSetIoType(deviceInit, WdfDeviceIoDirect);

	WDF_PNPPOWER_EVENT_CALLBACKS powerCallbacks;
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&powerCallbacks);

	powerCallbacks.EvtDevicePrepareHardware = DevicePrepareHardware;
	powerCallbacks.EvtDeviceReleaseHardware = DeviceReleaseHardware;
	powerCallbacks.EvtDeviceD0Entry = DeviceD0Entry;
	powerCallbacks.EvtDeviceD0Exit = DeviceD0Exit;

	WdfDeviceInitSetPnpPowerEventCallbacks(deviceInit, &powerCallbacks);

	WDF_FILEOBJECT_CONFIG fileConfig;
	WDF_FILEOBJECT_CONFIG_INIT(&fileConfig, DeviceFileCreate, DeviceFileClose, WDF_NO_EVENT_CALLBACK);

	WDF_OBJECT_ATTRIBUTES fileAttributes;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fileAttributes, FileObject);
	fileAttributes.EvtDestroyCallback = FileObjectDestroy;

	WdfDeviceInitSetFileObjectConfig(deviceInit, &fileConfig, &fileAttributes);

	WDF_OBJECT_ATTRIBUTES deviceAttributes;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, Device);

	deviceAttributes.EvtCleanupCallback = DeviceCleanup;
	deviceAttributes.SynchronizationScope = WdfSynchronizationScopeDevice;

	WDFDEVICE wdfDevice;
	if (!NT_SUCCESS(status = WdfDeviceCreate(&deviceInit, &deviceAttributes, &wdfDevice)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfDeviceCreate failed %!STATUS!", status);
		return status;
	}

	// Construct device object on the preallocated buffer using placement 'new'
	auto device = new (GetDeviceObject(wdfDevice)) Device(wdfDevice);
	if (device == nullptr)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "DeviceAdd failed %!STATUS!", status);
		return status;
	}

	return device->Initialize(deviceInit);
}

void DeviceCleanup(WDFOBJECT wdfObject)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	PAGED_CODE();

	Device* device = GetDeviceObject(wdfObject);

	NT_ASSERT(device != nullptr);

	if (device->m_device == wdfObject)
	{
		// Device object constructed using placement 'new' so explicitly invoke destructor
		device->Deinitialize();
		device->~Device();
	}
}

NTSTATUS DeviceD0Entry(WDFDEVICE wdfDevice, WDF_POWER_DEVICE_STATE previousState)
{
	PAGED_CODE();

	Device* device = GetDeviceObject(wdfDevice);

	NT_ASSERT(device != nullptr);

	return device->OnD0Entry(previousState);
}

NTSTATUS DeviceD0Exit(WDFDEVICE wdfDevice, WDF_POWER_DEVICE_STATE targetState)
{
	PAGED_CODE();

	Device* device = GetDeviceObject(wdfDevice);

	NT_ASSERT(device != nullptr);

	return device->OnD0Exit(targetState);
}

NTSTATUS DevicePrepareHardware(WDFDEVICE wdfDevice, WDFCMRESLIST resourcesRaw, WDFCMRESLIST resourcesTranslated)
{
	PAGED_CODE();

	Device* device = GetDeviceObject(wdfDevice);

	NT_ASSERT(device != nullptr);

	return device->OnPrepareHardware(resourcesRaw, resourcesTranslated);
}

NTSTATUS DeviceReleaseHardware(WDFDEVICE wdfDevice, WDFCMRESLIST resourcesTranslated)
{
	PAGED_CODE();

	Device* device = GetDeviceObject(wdfDevice);

	NT_ASSERT(device != nullptr);

	return device->OnReleaseHardware(resourcesTranslated);
}

BOOLEAN DeviceInterruptIsr(WDFINTERRUPT interrupt, ULONG messageID)
{
	UNREFERENCED_PARAMETER(messageID);

	return WdfInterruptQueueDpcForIsr(interrupt);
}

void DeviceInterruptDpc(WDFINTERRUPT interrupt, WDFOBJECT associatedObject)
{
	UNREFERENCED_PARAMETER(associatedObject);

	PAGED_CODE();

	Device* device = GetDeviceObject(associatedObject);

	NT_ASSERT(device != nullptr);

	device->OnInterrupt(interrupt);
}

void DeviceFileCreate(WDFDEVICE wdfDevice, WDFREQUEST wdfRequest, WDFFILEOBJECT wdfFileObject)
{
	UNREFERENCED_PARAMETER(wdfDevice);

	PAGED_CODE();

	Device* device = GetDeviceObject(wdfDevice);

	NT_ASSERT(device != nullptr);

	device->OnFileCreate(wdfRequest, wdfFileObject);
}

void DeviceFileClose(WDFFILEOBJECT wdfFileObject)
{
	PAGED_CODE();

	Device* device = GetDeviceObject(WdfFileObjectGetDevice(wdfFileObject));

	NT_ASSERT(device != nullptr);

	device->OnFileClose(wdfFileObject);
}


Device::Device(WDFDEVICE wdfDevice) :
	m_device(wdfDevice),
	m_queue(nullptr)
{
	NT_ASSERT(m_device != nullptr);
}

Device::~Device()
{
	m_device = nullptr;
}

NTSTATUS Device::Initialize(PWDFDEVICE_INIT deviceInit)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	UNREFERENCED_PARAMETER(deviceInit);

	NTSTATUS status = STATUS_SUCCESS;

	WDF_IO_QUEUE_CONFIG queueConfig;
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);

	queueConfig.EvtIoDeviceControl = QueueIoDeviceControl;

	WDF_OBJECT_ATTRIBUTES queueAttributes;
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes, Queue);

	WDFQUEUE wdfQueue;
	if (!NT_SUCCESS(status = WdfIoQueueCreate(m_device, &queueConfig, &queueAttributes, &wdfQueue)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfIoQueueCreate failed %!STATUS!", status);
		return status;
	}

	// Construct a queue object on the preallocated buffer using placement new operation
	m_queue = new (GetQueueObject(wdfQueue)) Queue(wdfQueue);
	if (m_queue == nullptr)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "Device::Initialize failed %!STATUS!", status);
		return status;
	}

	WDF_DEVICE_STATE deviceState;
	WDF_DEVICE_STATE_INIT(&deviceState);
	deviceState.NotDisableable = WdfFalse;

	WdfDeviceSetDeviceState(m_device, &deviceState);

	return m_queue->Initialize();
}

NTSTATUS Device::Deinitialize()
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	if (m_queue != nullptr)
	{
		// Queue object constructed using placement new so explicitly invoke destructor
		status = m_queue->Deinitialize();
		m_queue->~Queue();
		m_queue = nullptr;
	}

	return status;
}

NTSTATUS Device::OnD0Entry(WDF_POWER_DEVICE_STATE previousState)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	UNREFERENCED_PARAMETER(previousState);

	NTSTATUS status = STATUS_SUCCESS;
	if (!NT_SUCCESS(status = WdfIoTargetStart(m_serialTarget)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfIoTargetStart (Serial) failed %!STATUS!", status);
		return status;
	}

	if (!NT_SUCCESS(status = WdfIoTargetStart(m_gpioTarget)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfIoTargetStart (GPIO) failed %!STATUS!", status);
		return status;
	}

	return InitializePCD();
}

NTSTATUS Device::OnD0Exit(WDF_POWER_DEVICE_STATE targetState)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	UNREFERENCED_PARAMETER(targetState);

	NTSTATUS status = DeinitializePCD();

	WdfIoTargetStop(m_serialTarget, WdfIoTargetCancelSentIo);
	WdfIoTargetStop(m_gpioTarget, WdfIoTargetCancelSentIo);

	return status;
}

NTSTATUS Device::OnPrepareHardware(WDFCMRESLIST resourcesRaw, WDFCMRESLIST resourcesTranslated)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	auto resourceCount = WdfCmResourceListGetCount(resourcesTranslated);

	BYTE flags = 0;
	for (ULONG i = 0; i < resourceCount; i++)
	{
		auto descriptor = WdfCmResourceListGetDescriptor(resourcesTranslated, i);
		switch (descriptor->Type)
		{
		case CmResourceTypeConnection:
			if (descriptor->u.Connection.Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL &&
				(descriptor->u.Connection.Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_SPI || descriptor->u.Connection.Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_I2C))
			{
				ReturnIfFailed(status, CreateTarget(descriptor, &m_serialTarget));
				flags |= DEVICE_RESOURCE_SERIAL;
			}
			else if (descriptor->u.Connection.Class == CM_RESOURCE_CONNECTION_CLASS_GPIO &&
				descriptor->u.Connection.Type == CM_RESOURCE_CONNECTION_TYPE_GPIO_IO)
			{
				ReturnIfFailed(status, CreateTarget(descriptor, &m_gpioTarget));
				flags |= DEVICE_RESOURCE_GPIO;
			}
			break;
		case CmResourceTypeInterrupt:
			ReturnIfFailed(status, CreateInterrupt(WdfCmResourceListGetDescriptor(resourcesRaw, i), descriptor, &m_interrupt));
			flags |= DEVICE_RESOURCE_INTERRUPT;
			break;
		}
	}

	if ((flags & DEVICE_RESOURCES) != DEVICE_RESOURCES)
	{
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}

NTSTATUS Device::OnReleaseHardware(WDFCMRESLIST resourcesTranslated)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	UNREFERENCED_PARAMETER(resourcesTranslated);

	m_serialTarget.Close();
	m_gpioTarget.Close();
	m_interrupt.Close();

	return STATUS_SUCCESS;
}

NTSTATUS Device::OnInterrupt(WDFINTERRUPT interrupt)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	UNREFERENCED_PARAMETER(interrupt);

	BYTE value;
	ReadRegister(PCDRegister_ComIrqReg, &value);

	return STATUS_SUCCESS;
}

NTSTATUS Device::OnFileCreate(WDFREQUEST wdfRequest, WDFFILEOBJECT wdfFileObject)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	UNREFERENCED_PARAMETER(wdfRequest);
	UNREFERENCED_PARAMETER(wdfFileObject);

	return STATUS_SUCCESS;

	//return m_queue->OnFileCreate(wdfRequest, wdfFileObject);
}

NTSTATUS Device::OnFileClose(WDFFILEOBJECT wdfFileObject)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	UNREFERENCED_PARAMETER(wdfFileObject);

	return STATUS_SUCCESS;

	//return m_queue->OnFileClose(wdfFileObject);
}

NTSTATUS Device::CreateTarget(PCM_PARTIAL_RESOURCE_DESCRIPTOR translatedDescriptor, WDFIOTARGET* targetResource)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;
	DECLARE_UNICODE_STRING_SIZE(resourcePath, RESOURCE_HUB_PATH_CHARS);

	if (!NT_SUCCESS(status = RESOURCE_HUB_CREATE_PATH_FROM_ID(&resourcePath, translatedDescriptor->u.Connection.IdLowPart, translatedDescriptor->u.Connection.IdHighPart)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "RESOURCE_HUB_CREATE_PATH_FROM_ID failed %!STATUS!", status);
		return status;
	}

	WdfHandle<WDFIOTARGET> target;
	if (!NT_SUCCESS(status = WdfIoTargetCreate(m_device, WDF_NO_OBJECT_ATTRIBUTES, &target)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfIoTargetCreate failed %!STATUS!", status);
		return status;
	}

	WDF_IO_TARGET_OPEN_PARAMS openParams;
	WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&openParams, &resourcePath, GENERIC_READ | GENERIC_WRITE);

	if (!NT_SUCCESS(status = WdfIoTargetOpen(target, &openParams)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfIoTargetOpen failed %!STATUS!", status);
		return status;
	}

	*targetResource = target.Detach();
	return status;
}

NTSTATUS Device::CreateInterrupt(PCM_PARTIAL_RESOURCE_DESCRIPTOR rawDescriptor, PCM_PARTIAL_RESOURCE_DESCRIPTOR translatedDescriptor, WDFINTERRUPT* interrupt)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	WDF_INTERRUPT_CONFIG interruptConfiguration;
	WDF_INTERRUPT_CONFIG_INIT(&interruptConfiguration, DeviceInterruptIsr, DeviceInterruptDpc);

	interruptConfiguration.InterruptRaw = rawDescriptor;
	interruptConfiguration.InterruptTranslated = translatedDescriptor;
	interruptConfiguration.AutomaticSerialization = TRUE;

	if (!NT_SUCCESS(status = WdfInterruptCreate(m_device, &interruptConfiguration, WDF_NO_OBJECT_ATTRIBUTES, interrupt)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfInterruptCreate failed %!STATUS!", status);
		return status;
	}

	return status;
}

NTSTATUS Device::ReadGPIO(BOOLEAN* value)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	WDF_MEMORY_DESCRIPTOR memoryDescriptor;
	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memoryDescriptor, value, sizeof(BOOLEAN));

	ULONG_PTR bytesTransfered;
	if (!NT_SUCCESS(status = WdfIoTargetSendIoctlSynchronously(m_gpioTarget, nullptr, IOCTL_GPIO_READ_PINS, nullptr, &memoryDescriptor, nullptr, &bytesTransfered)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfIoTargetSendIoctlSynchronously failed %!STATUS!", status);
		return status;
	}

#if DEBUG
	if (bytesTransfered != sizeof(BOOLEAN))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "Reading GPIO transfered only %d bytes of %d", bytesTransfered, sizeof(BOOLEAN));
		status = STATUS_DEVICE_DATA_ERROR;
		return status;
	}
#endif

	return STATUS_SUCCESS;
}

NTSTATUS Device::WriteGPIO(BOOLEAN value)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	WDF_MEMORY_DESCRIPTOR memoryDescriptor;
	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memoryDescriptor, &value, sizeof(BOOLEAN));

	ULONG_PTR bytesTransfered;
	if (!NT_SUCCESS(status = WdfIoTargetSendIoctlSynchronously(m_gpioTarget, nullptr, IOCTL_GPIO_WRITE_PINS, &memoryDescriptor, nullptr, nullptr, &bytesTransfered)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfIoTargetSendIoctlSynchronously failed %!STATUS!", status);
		return status;
	}

#if DEBUG
	if (bytesTransfered != sizeof(BOOLEAN))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "Writing GPIO transfered only %d bytes of %d", bytesTransfered, sizeof(BOOLEAN));
		status = STATUS_DEVICE_DATA_ERROR;
		return status;
	}
#endif

	return STATUS_SUCCESS;
}

NTSTATUS Device::ReadRegister(PCDRegister reg, BYTE* buffer, ULONG length)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	SPB_TRANSFER_LIST_AND_ENTRIES(2) transferList;
	SPB_TRANSFER_LIST_INIT(&transferList.List, 2);

	BYTE address = (reg << 1) | 0x80;
	transferList.List.Transfers[0] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(SpbTransferDirectionToDevice, 0, &address, sizeof(BYTE));
	transferList.List.Transfers[1] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(SpbTransferDirectionFromDevice, 0, buffer, length);

	WDF_MEMORY_DESCRIPTOR sequenceBufferDescriptor;
	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&sequenceBufferDescriptor, &transferList, sizeof(transferList));

	ULONG_PTR bytesTransfered;
	if (!NT_SUCCESS(status = WdfIoTargetSendIoctlSynchronously(m_serialTarget, nullptr,
		IOCTL_SPB_EXECUTE_SEQUENCE, &sequenceBufferDescriptor, nullptr, nullptr, &bytesTransfered)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfIoTargetSendIoctlSynchronously failed %!STATUS!", status);
		return status;
	}

#if DEBUG
	if (bytesTransfered != length + sizeof(BYTE))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "Reading register 0x%x transfered only %d bytes of %d", reg, bytesTransfered, length + sizeof(BYTE));
		status = STATUS_DEVICE_DATA_ERROR;
		return status;
	}
#endif

	return STATUS_SUCCESS;
}

NTSTATUS Device::ReadRegister(PCDRegister reg, BYTE* buffer, ULONG length, BYTE align)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	BYTE mask = 0;
	for (BYTE i = align; i < 8; i++)
	{
		mask |= (1 << i);
	}

	BYTE firstByte = (buffer[0] & ~mask);

	ReturnIfFailed(status, ReadRegister(reg, buffer, length));
	buffer[0] = firstByte | (buffer[0] & mask);

	return STATUS_SUCCESS;
}

NTSTATUS Device::WriteRegister(PCDRegister reg, BYTE* buffer, ULONG length)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	SPB_TRANSFER_LIST_AND_ENTRIES(2) transferList;
	SPB_TRANSFER_LIST_INIT(&transferList.List, 2);

	BYTE address = reg << 1;
	transferList.List.Transfers[0] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(SpbTransferDirectionToDevice, 0, &address, sizeof(BYTE));
	transferList.List.Transfers[1] = SPB_TRANSFER_LIST_ENTRY_INIT_SIMPLE(SpbTransferDirectionToDevice, 0, buffer, length);

	WDF_MEMORY_DESCRIPTOR sequenceBufferDescriptor;
	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&sequenceBufferDescriptor, &transferList, sizeof(transferList));

	ULONG_PTR bytesTransfered;
	if (!NT_SUCCESS(status = WdfIoTargetSendIoctlSynchronously(m_serialTarget, nullptr,
		IOCTL_SPB_EXECUTE_SEQUENCE, &sequenceBufferDescriptor, nullptr, nullptr, &bytesTransfered)))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfIoTargetSendIoctlSynchronously failed %!STATUS!", status);
		return status;
	}

#if DEBUG
	if (bytesTransfered != length + sizeof(BYTE))
	{
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "Writing register 0x%x transfered only %d bytes of %d", reg, bytesTransfered, length + sizeof(BYTE));
		status = STATUS_DEVICE_DATA_ERROR;
		return status;
	}
#endif

	return STATUS_SUCCESS;
}

NTSTATUS Device::InitializePCD()
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	//BOOLEAN gpioValue;
	//ReturnIfFailed(status, ReadGPIO(&gpioValue));

	//if (gpioValue)
	//{
	//	ReturnIfFailed(status, Reset());
	//}
	//else
	//{
	//	ReturnIfFailed(status, WriteGPIO(TRUE));


	//	Sleep(50);
	//}

	ReturnIfFailed(status, WriteGPIO(FALSE));
	ReturnIfFailed(status, WriteGPIO(TRUE));

	Sleep(50);

	//Timer = (2 * TPreScaler + 1) / 13.56 MHz where TPreScaler = [TPrescaler_Hi:TPrescaler_Lo]
	ReturnIfFailed(status, WriteRegister(PCDRegister_TModeReg, 0x80));
	ReturnIfFailed(status, WriteRegister(PCDRegister_TPrescalerReg, 0xA9));
	ReturnIfFailed(status, WriteRegister(PCDRegister_TReloadRegH, 0x03));
	ReturnIfFailed(status, WriteRegister(PCDRegister_TReloadRegL, 0xE8));
	ReturnIfFailed(status, WriteRegister(PCDRegister_TxASKReg, 0x40));
	ReturnIfFailed(status, WriteRegister(PCDRegister_ModeReg, 0x3D));
	ReturnIfFailed(status, SetRegisterBitMask(PCDRegister_TxControlReg, 0x03));

	ReturnIfFailed(status, WriteRegister(PCDRegister_ComIEnReg, 0xB0));
	ReturnIfFailed(status, WriteRegister(PCDRegister_DivIEnReg, 0x84));

	BOOLEAN hasNewCard = FALSE;
	for (ULONG i = 0; i < 0xFF; i++)
	{
		HasNewCard(&hasNewCard);

		if (hasNewCard)
		{
			break;
		}

		Sleep(50);
	}

	if (hasNewCard)
	{
		BYTE sak;
		BYTE cartdUId[UID_MAX_LENGTH];
		ULONG cartdUIdLength = UID_MAX_LENGTH;

		ReturnIfFailed(status, Select(cartdUId, &cartdUIdLength, &sak));
	}

	return STATUS_SUCCESS;
}

NTSTATUS Device::DeinitializePCD()
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	return WriteGPIO(FALSE);
}

NTSTATUS Device::Reset()
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	ReturnIfFailed(status, WriteRegister(PCDRegister_CommandReg, PCDCommand_SoftReset));

	Sleep(50);

	BYTE resetValue;

	do
	{
		ReturnIfFailed(status, ReadRegister(PCDRegister_CommandReg, &resetValue));
	} while (resetValue & 0x10);

	return STATUS_SUCCESS;
}

NTSTATUS Device::CalculateCRC(BYTE* buffer, ULONG length, BYTE* crcBuffer)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	//ReturnIfFailed(status, WriteRegister(PCDRegister_CommandReg, PCDCommand_Idle));
	ReturnIfFailed(status, ClearRegisterBitMask(PCDRegister_DivIrqReg, 0x04));
	ReturnIfFailed(status, SetRegisterBitMask(PCDRegister_FIFOLevelReg, 0x80));
	ReturnIfFailed(status, WriteRegister(PCDRegister_FIFODataReg, buffer, length));
	ReturnIfFailed(status, WriteRegister(PCDRegister_CommandReg, PCDCommand_CalcCRC));

	BYTE value;
	ULONG timeout = 2000;
	while (true)
	{
		ReturnIfFailed(status, ReadRegister(PCDRegister_DivIrqReg, &value));
		if ((value & 0x04) != 0)
		{
			break;
		}
		else if (--timeout == 0)
		{
			return STATUS_TRANSACTION_TIMED_OUT;
		}
	}

#if DEBUG
	BYTE error;
	ReturnIfFailed(status, ReadRegister(PCDRegister_ErrorReg, &error));

	if (error & 0x4)
	{
		return STATUS_CRC_ERROR;
	}
#endif

	ReturnIfFailed(status, WriteRegister(PCDRegister_CommandReg, PCDCommand_Idle));

	ReturnIfFailed(status, ReadRegister(PCDRegister_CRCResultRegL, crcBuffer));
	ReturnIfFailed(status, ReadRegister(PCDRegister_CRCResultRegH, crcBuffer + 1));

	return STATUS_SUCCESS;
}

NTSTATUS Device::SelfTest()
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	ReturnIfFailed(status, Reset());

	BYTE zeroes[25] = { 0x00 };
	ReturnIfFailed(status, SetRegisterBitMask(PCDRegister_FIFOLevelReg, 0x80));
	ReturnIfFailed(status, WriteRegister(PCDRegister_FIFODataReg, zeroes, 25));
	ReturnIfFailed(status, WriteRegister(PCDRegister_CommandReg, PCDCommand_Mem));
	ReturnIfFailed(status, WriteRegister(PCDRegister_AutoTestReg, 0x09));
	ReturnIfFailed(status, WriteRegister(PCDRegister_FIFODataReg, 0x00));
	ReturnIfFailed(status, WriteRegister(PCDRegister_CommandReg, PCDCommand_CalcCRC));

	BYTE fifoLevel;
	for (ULONG i = 0; i < 0xFF; i++)
	{
		ReturnIfFailed(status, ReadRegister(PCDRegister_FIFOLevelReg, &fifoLevel));

		if (fifoLevel >= 64)
		{
			break;
		}
	}

#if DEBUG
	BYTE error;
	ReturnIfFailed(status, ReadRegister(PCDRegister_ErrorReg, &error));

	if (error & 0x4)
	{
		return STATUS_CRC_ERROR;
	}
#endif

	ReturnIfFailed(status, WriteRegister(PCDRegister_CommandReg, PCDCommand_Idle));

	BYTE result[64];
	ReturnIfFailed(status, ReadRegister(PCDRegister_FIFODataReg, result, 64));
	ReturnIfFailed(status, WriteRegister(PCDRegister_AutoTestReg, 0x00));

	BYTE version;
	ReturnIfFailed(status, ReadRegister(PCDRegister_VersionReg, &version));

	switch (version)
	{
	case 0x88:	// Fudan Semiconductor FM17522 clone
		break;
	case 0x90:	// Version 0.0
		break;
	case 0x91:	// Version 1.0
		break;
	case 0x92:	// Version 2.0
		break;
	default:
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}

NTSTATUS Device::CommunicateWithPICC(PCDCommand command, BYTE waitIRq, BYTE* inputBuffer, ULONG inputLength, BYTE* outputBuffer, ULONG* outputLength, BYTE align, BYTE* lastBits)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	ReturnIfFailed(status, ClearRegisterBitMask(PCDRegister_CollReg, 0x80));
	ReturnIfFailed(status, WriteRegister(PCDRegister_ComIrqReg, 0x7F));
	ReturnIfFailed(status, SetRegisterBitMask(PCDRegister_FIFOLevelReg, 0x80));
	ReturnIfFailed(status, WriteRegister(PCDRegister_FIFODataReg, inputBuffer, inputLength));

	BYTE framing = (align << 4) | (lastBits == nullptr ? 0 : *lastBits);
	if (command == PCDCommand_Transceive)
	{
		framing |= 0x80;
	}

	ReturnIfFailed(status, WriteRegister(PCDRegister_BitFramingReg, framing));
	ReturnIfFailed(status, WriteRegister(PCDRegister_CommandReg, command));

	BYTE value;
	ULONG timeout = 2000;
	while (true)
	{
		ReturnIfFailed(status, ReadRegister(PCDRegister_ComIrqReg, &value));
		if ((value & waitIRq) != 0)
		{
			break;
		}
		else if ((value & 0x01) != 0 || --timeout == 0)
		{
			return STATUS_TRANSACTION_TIMED_OUT;
		}
	}

	ReturnIfFailed(status, ClearRegisterBitMask(PCDRegister_BitFramingReg, 0x80));

	BYTE error;
	ReturnIfFailed(status, ReadRegister(PCDRegister_ErrorReg, &error));
	if (error & 0x13)
	{
		if (error & 0x2)
		{
			return STATUS_PARITY_ERROR;
		}
		else if (error & 0x10)
		{
			return STATUS_BUFFER_OVERFLOW;
		}
		else
		{
			return STATUS_UNSUCCESSFUL;
		}
	}

	BYTE fifoLevel;
	ReturnIfFailed(status, ReadRegister(PCDRegister_FIFOLevelReg, &fifoLevel));

	if (*outputLength < fifoLevel)
		return STATUS_BUFFER_TOO_SMALL;

	*outputLength = fifoLevel;
	ReturnIfFailed(status, ReadRegister(PCDRegister_FIFODataReg, outputBuffer, fifoLevel, align));

	if (lastBits != nullptr)
	{
		BYTE rxLastBits;
		ReturnIfFailed(status, ReadRegister(PCDRegister_ControlReg, &rxLastBits));

		*lastBits = rxLastBits & 0x07;
	}

	if (error & 0x08)
	{
		return STATUS_CARD_COLLISION;
	}

	return STATUS_SUCCESS;
}

NTSTATUS Device::Select(BYTE* cardUIdBuffer, ULONG* cardUIdLength, BYTE* sak)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	ReturnIfFailed(status, ClearRegisterBitMask(PCDRegister_CollReg, 0x80));

	BYTE count;
	BYTE rxAlign;
	BYTE cascadeLevel = 1;
	BYTE validBits = 0;
	BYTE txLastBits = 0;
	BYTE uid[UID_MAX_LENGTH];
	BYTE uidIndex = 0;
	ULONG uidLength = 0;
	BYTE buffer[9];
	ULONG bufferLength = 0;
	BYTE* responseBuffer = nullptr;
	ULONG responseLength = 0;
	BOOLEAN uidComplete = FALSE;
	BOOLEAN useCascadeTag = FALSE;

	while (!uidComplete)
	{
		switch (cascadeLevel)
		{
		case 1:
			buffer[0] = PICCCommand_SelCL1;
			uidIndex = 0;
			useCascadeTag = validBits != 0 && uidLength > 4;
			break;

		case 2:
			buffer[0] = PICCCommand_SelCL2;
			uidIndex = 3;
			useCascadeTag = validBits != 0 && uidLength > 7;
			break;

		case 3:
			buffer[0] = PICCCommand_SelCL3;
			uidIndex = 6;
			useCascadeTag = FALSE;
			break;
		default:
			return STATUS_UNSUCCESSFUL;
		}

		INT8 currentLevelKnownBits = validBits - (8 * uidIndex);
		if (currentLevelKnownBits < 0)
		{
			currentLevelKnownBits = 0;
		}

		BYTE index = 2;
		if (useCascadeTag)
			buffer[index++] = PICCCommand_CT;

		ULONG bytesToCopy = (currentLevelKnownBits / 8 + ((currentLevelKnownBits % 8) == 0 ? 0 : 1));
		if (bytesToCopy > 0)
		{
			ULONG maxBytes = useCascadeTag ? 3 : 4;
			if (bytesToCopy > maxBytes)
			{
				bytesToCopy = maxBytes;
			}

			for (count = 0; count < bytesToCopy; count++)
			{
				buffer[index++] = uid[uidIndex + count];
			}
		}

		if (useCascadeTag)
		{
			currentLevelKnownBits += 8;
		}

		BOOLEAN selectDone = FALSE;
		while (!selectDone)
		{
			if (currentLevelKnownBits >= 32)
			{
				buffer[1] = 0x70;
				buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5];

				ReturnIfFailed(status, CalculateCRC(buffer, 7, &buffer[7]));

				txLastBits = 0;
				bufferLength = 9;
				responseBuffer = &buffer[6];
				responseLength = 3;
			}
			else
			{
				txLastBits = currentLevelKnownBits % 8;
				count = currentLevelKnownBits / 8;
				index = 2 + count;
				buffer[1] = (index << 4) | txLastBits;
				bufferLength = index + (txLastBits ? 1 : 0);
				responseBuffer = &buffer[index];
				responseLength = 9 - index;
			}

			rxAlign = txLastBits;

			ReturnIfFailed(status, WriteRegister(PCDRegister_BitFramingReg, (rxAlign << 4) | txLastBits));

			status = CommunicateWithPICC(PCDCommand_Transceive, 0x30, buffer, bufferLength, responseBuffer, &responseLength, rxAlign, &txLastBits);
			if (status == STATUS_CARD_COLLISION)
			{
				BYTE collision;
				ReturnIfFailed(status, ReadRegister(PCDRegister_CollReg, &collision));

				if (collision & 0x20)
					return STATUS_CARD_COLLISION;

				BYTE collisionPosition = collision & 0x1F;
				if (collisionPosition == 0)
					collisionPosition = 32;

				if (collisionPosition <= currentLevelKnownBits)
					return STATUS_INTERNAL_ERROR;

				currentLevelKnownBits = collisionPosition;
				count = (currentLevelKnownBits - 1) % 8;
				index = 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0);
				buffer[index] |= (1 << count);
			}
			else if (status != STATUS_SUCCESS)
			{
				return status;
			}
			else
			{
				if (currentLevelKnownBits >= 32)
				{
					selectDone = TRUE;
				}
				else
				{
					currentLevelKnownBits = 32;
				}
			}
		}

		index = buffer[2] == PICCCommand_CT ? 3 : 2;
		bytesToCopy = buffer[2] == PICCCommand_CT ? 3 : 4;

		for (count = 0; count < bytesToCopy; count++)
		{
			uid[uidIndex + count] = buffer[index++];
		}

		if (responseLength != 3 || txLastBits != 0)
		{
			return STATUS_UNSUCCESSFUL;
		}

		ReturnIfFailed(status, CalculateCRC(responseBuffer, 1, &buffer[2]));

		if ((buffer[2] != responseBuffer[1]) || (buffer[3] != responseBuffer[2]))
		{
			return STATUS_CRC_ERROR;
		}

		if (responseBuffer[0] & 0x04)
		{
			cascadeLevel++;
		}
		else
		{
			uidComplete = TRUE;
			*sak = responseBuffer[0];
		}
	}

	uidLength = 3 * cascadeLevel + 1;
	if (*cardUIdLength < uidLength)
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	*cardUIdLength = uidLength;
	CopyMemory(cardUIdBuffer, uid, uidLength);
	return STATUS_SUCCESS;
}

NTSTATUS Device::HasNewCard(BOOLEAN* value)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	NTSTATUS status = STATUS_SUCCESS;

	ReturnIfFailed(status, ClearRegisterBitMask(PCDRegister_CollReg, 0x80));

	BYTE validBits = 7;
	BYTE bufferATQA[2];
	ULONG bufferATQALength = 2;
	BYTE command = PICCCommand_ReqA;
	ReturnIfFailed(status, CommunicateWithPICC(PCDCommand_Transceive, 0x30, &command, 1, bufferATQA, &bufferATQALength, 0, &validBits));

	if (bufferATQALength != 2 || validBits != 0)
	{
		return STATUS_UNSUCCESSFUL;
	}

	*value = status == STATUS_SUCCESS || status == STATUS_CARD_COLLISION;
	return STATUS_SUCCESS;
}