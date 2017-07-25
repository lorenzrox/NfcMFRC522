#pragma once
#include <ntassert.h>
#include <wdfobject.h>

template<typename T>
class WdfHandle
{
public:
	explicit WdfHandle(_In_opt_ typename T handle = nullptr)  throw() :
		m_handle(handle)
	{
	}

	WdfHandle(_Inout_ WdfHandle&& handle)  throw() :
		m_handle(handle.m_handle)
	{
		handle.m_handle = nullptr;
	}

	~WdfHandle() throw()
	{
		Close();
	}

	inline WdfHandle& operator=(_Inout_ WdfHandle&& handle) throw()
	{
		Close();

		m_handle = handle.m_handle;
		handle.m_handle = nullptr;
		return *this;
	}

	inline WdfHandle& operator=(decltype(__nullptr)) throw()
	{
		Close();
		return *this;
	}

	inline typename T* operator&() throw()
	{
		return CloseAndGetAddressOf();
	}

	inline const typename T* operator&() const throw()
	{
		return GetAddressOf();
	}

	inline operator typename T() throw()
	{
		return m_handle;
	}

	inline operator WDFOBJECT() throw()
	{
		return m_handle;
	}

	inline operator typename T*() throw()
	{
		return CloseAndGetAddressOf();
	}

	inline operator bool() const throw()
	{
		return m_handle != nullptr;
	}

	inline operator void*() const throw()
	{
		return m_handle;
	}

	inline void Attach(typename T* handle) throw()
	{
		if (handle != m_handle)
		{
			Close();
			m_handle = handle;
		}
	}

	inline typename T Detach() throw()
	{
		typename T handle = m_handle;
		m_handle = nullptr;

		return handle;
	}

	inline typename T Get() const throw()
	{
		return m_handle;
	}

	typename T* CloseAndGetAddressOf() throw()
	{
		Close();
		return &m_handle;
	}

	const typename T* GetAddressOf() const throw()
	{
		return &m_handle;
	}

	void Close() throw()
	{
		if (m_handle != nullptr)
		{
			WdfObjectDelete(m_handle);
			m_handle = nullptr;
		}
	}

private:
	typename T m_handle;
};

template<typename T>
inline bool operator==(const WdfHandle<T>& a, const WdfHandle<T>& b) throw()
{
	return a.Get() == b.Get();
}

template<typename T>
inline bool operator==(const WdfHandle<T>& a, decltype(__nullptr)) throw()
{
	return a.Get() == nullptr;
}

template<typename T>
inline bool operator==(decltype(__nullptr), const WdfHandle<T>& a) throw()
{
	return a.Get() == nullptr;
}

template<class T>
inline bool operator!=(const WdfHandle<T>& a, decltype(__nullptr)) throw()
{
	return a.Get() != nullptr;
}

template<class T>
inline bool operator!=(decltype(__nullptr), const WdfHandle<T>& a) throw()
{
	return a.Get() != nullptr;
}