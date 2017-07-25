#pragma once
#include "Driver.h"

EXTERN_C_START

EVT_WDF_OBJECT_CONTEXT_DESTROY FileObjectDestroy;
EVT_WDF_REQUEST_CANCEL FileRequestCancel;

EXTERN_C_END

class Device;

class FileObject
{
	friend class Device;
	friend EVT_WDF_OBJECT_CONTEXT_DESTROY FileObjectDestroy;
	friend EVT_WDF_REQUEST_CANCEL FileRequestCancel;

public:
	FileObject(_In_ WDFFILEOBJECT wdfFileObject);
	~FileObject();

	Device* GetDevice() const;

private:
	WDFFILEOBJECT m_fileObject;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FileObject, GetFileObject);