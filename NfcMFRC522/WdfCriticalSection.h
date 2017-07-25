#pragma once
#include <wdftypes.h>
#include <wdfsync.h>
#include <wudfwdm.h>
#include "WdfHandle.h"

#include <wrl\wrappers\corewrappers.h>

class WdfCriticalSection;

namespace Details
{

	class WdfCriticalSectionLock
	{
		friend class WdfCriticalSection;

	public:
		WdfCriticalSectionLock(_Inout_ WdfCriticalSectionLock&& other) throw() :
			m_handle(other.m_handle)
		{
			other.m_handle = nullptr;
		}

		~WdfCriticalSectionLock() throw()
		{
			Unlock();
		}

		void Unlock() throw()
		{
			if (m_handle != nullptr)
			{
				WdfWaitLockRelease(*m_handle);
				m_handle = nullptr;
			}
		}

		inline bool IsLocked() const throw()
		{
			return m_handle != nullptr;
		}

	private:
		explicit WdfCriticalSectionLock(WDFWAITLOCK const* handle = nullptr) throw() :
			m_handle(handle)
		{
		}

		WDFWAITLOCK const* m_handle;
	};
}

class WdfCriticalSection : public WdfHandle<WDFWAITLOCK>
{
public:
	typedef Details::WdfCriticalSectionLock SyncLock;

	explicit WdfCriticalSection(_In_opt_ WDFWAITLOCK handle = nullptr) throw() :
		WdfHandle(handle)
	{
	}

	inline static NTSTATUS Create(_Out_ WdfCriticalSection& criticalSection)
	{
		return Create(WDF_NO_OBJECT_ATTRIBUTES, criticalSection);
	}

	inline static NTSTATUS Create(PWDF_OBJECT_ATTRIBUTES attributes, _Out_ WdfCriticalSection& criticalSection)
	{
		return WdfWaitLockCreate(attributes, &criticalSection);
	}

	SyncLock Lock(_In_opt_ PLONGLONG timeout = nullptr) throw()
	{
		WdfWaitLockAcquire(Get(), timeout);

		return SyncLock(GetAddressOf());
	}

private:
	WdfCriticalSection(const WdfCriticalSection&);
	WdfCriticalSection& operator=(const WdfCriticalSection&);
	WDFWAITLOCK Detach();
	void Attach(WDFWAITLOCK);
	WDFWAITLOCK* GetAddressOf();
	WDFWAITLOCK* CloseAndGetAddressOf();
};

