#pragma once
#include "Driver.h"
#include "RC522.h"
#include "WdfCriticalSection.h"

EXTERN_C_START

EVT_WDF_DRIVER_DEVICE_ADD DeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP DeviceCleanup;
EVT_WDF_DEVICE_D0_ENTRY DeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT DeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE DevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE DeviceReleaseHardware;
EVT_WDF_INTERRUPT_ISR DeviceInterruptIsr;
EVT_WDF_INTERRUPT_DPC DeviceInterruptDpc;
EVT_WDF_DEVICE_FILE_CREATE DeviceFileCreate;
EVT_WDF_FILE_CLOSE DeviceFileClose;

EXTERN_C_END


class Queue;

class Device
{
	friend EVT_WDF_DRIVER_DEVICE_ADD DeviceAdd;
	friend EVT_WDF_OBJECT_CONTEXT_CLEANUP DeviceCleanup;
	friend EVT_WDF_DEVICE_D0_ENTRY DeviceD0Entry;
	friend EVT_WDF_DEVICE_D0_EXIT DeviceD0Exit;
	friend EVT_WDF_DEVICE_PREPARE_HARDWARE DevicePrepareHardware;
	friend EVT_WDF_DEVICE_RELEASE_HARDWARE DeviceReleaseHardware;
	friend EVT_WDF_INTERRUPT_ISR DeviceInterruptIsr;
	friend EVT_WDF_INTERRUPT_DPC DeviceInterruptDpc;
	friend EVT_WDF_DEVICE_FILE_CREATE DeviceFileCreate;
	friend EVT_WDF_FILE_CLOSE DeviceFileClose;

public:
	Device(_In_ WDFDEVICE wdfDevice);
	~Device();

	inline Queue* GetQueue() const
	{
		return m_queue;
	}

private:
	struct ConnectionParameters
	{
		UCHAR SerialType;
		ULARGE_INTEGER SerialConnectionId;
		ULARGE_INTEGER GPIOConnectionId;
	};

	NTSTATUS Initialize(_In_ PWDFDEVICE_INIT deviceInit);
	NTSTATUS Deinitialize();
	NTSTATUS OnD0Entry(_In_ WDF_POWER_DEVICE_STATE previousState);
	NTSTATUS OnD0Exit(_In_ WDF_POWER_DEVICE_STATE targetState);
	NTSTATUS OnPrepareHardware(_In_ WDFCMRESLIST resourcesRaw, _In_ WDFCMRESLIST resourcesTranslated);
	NTSTATUS OnReleaseHardware(_In_ WDFCMRESLIST resourcesTranslated);
	NTSTATUS OnInterrupt(_In_ WDFINTERRUPT interrupt);
	NTSTATUS OnFileCreate(_In_ WDFREQUEST wdfRequest, _In_ WDFFILEOBJECT wdfFileObject);
	NTSTATUS OnFileClose(_In_ WDFFILEOBJECT wdfFileObject);
	NTSTATUS CreateTarget(_In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR translatedDescriptor, _Out_ WDFIOTARGET* targetResource);
	NTSTATUS CreateInterrupt(_In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR rawDescriptor, _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR translatedDescriptor, _Out_ WDFINTERRUPT* interrupt);

	NTSTATUS ReadGPIO(_Out_ BOOLEAN* value);
	NTSTATUS WriteGPIO(BOOLEAN value);
	NTSTATUS ReadRegister(PCDRegister reg, _Out_writes_bytes_(length) BYTE* buffer, ULONG length);
	NTSTATUS ReadRegister(PCDRegister reg, _Out_writes_bytes_(length) BYTE* buffer, ULONG length, BYTE align);
	NTSTATUS WriteRegister(PCDRegister reg, _In_reads_bytes_(length) BYTE* buffer, ULONG length);
	NTSTATUS InitializePCD();
	NTSTATUS DeinitializePCD();
	NTSTATUS Reset();
	NTSTATUS CalculateCRC(_In_reads_bytes_(length) BYTE* buffer, ULONG length, _Out_writes_bytes_(2) BYTE* crcBuffer);
	NTSTATUS SelfTest();
	NTSTATUS CommunicateWithPICC(PCDCommand command, BYTE waitIRq, _In_reads_bytes_(inputLength) BYTE* inputBuffer, ULONG inputLength,
		_Out_writes_bytes_(*outputLength) BYTE* outputBuffer, ULONG* outputLength, BYTE align = NULL, BYTE* lastBits = nullptr);
	NTSTATUS HasNewCard(_Out_ BOOLEAN* value);
	NTSTATUS Select(_Out_writes_bytes_(*cardUIdLength) BYTE* cardUIdBuffer, ULONG* cardUIdLength, BYTE* sak);

	inline NTSTATUS ReadRegister(PCDRegister reg, BYTE* byte)
	{
		return ReadRegister(reg, byte, sizeof(BYTE));
	}

	inline NTSTATUS WriteRegister(PCDRegister reg, BYTE byte)
	{
		return WriteRegister(reg, &byte, sizeof(BYTE));
	}

	inline NTSTATUS SetRegisterBitMask(PCDRegister reg, BYTE mask)
	{
		NTSTATUS status = STATUS_SUCCESS;

		BYTE value;
		ReturnIfFailed(status, ReadRegister(reg, &value));

		return WriteRegister(reg, value | mask);
	}

	inline NTSTATUS ClearRegisterBitMask(PCDRegister reg, BYTE mask)
	{
		NTSTATUS status = STATUS_SUCCESS;

		BYTE value;
		ReturnIfFailed(status, ReadRegister(reg, &value));

		return WriteRegister(reg, value & ~mask);
	}

	inline NTSTATUS IsRegisterBitMaskSet(PCDRegister reg, BYTE mask, BOOLEAN* set)
	{
		NTSTATUS status = STATUS_SUCCESS;

		BYTE value;
		ReturnIfFailed(status, ReadRegister(reg, &value));

		*set = (value & mask) != 0;
		return STATUS_SUCCESS;
	}

	WDFDEVICE m_device;
	Queue* m_queue;
	WdfHandle<WDFIOTARGET> m_serialTarget;
	WdfHandle<WDFIOTARGET> m_gpioTarget;
	WdfHandle<WDFINTERRUPT> m_interrupt;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(Device, GetDeviceObject);