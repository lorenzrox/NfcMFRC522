#include "FileObject.h"
#include "FileObject.tmh"
#include "Device.h"

#pragma alloc_text(PAGE, FileObjectDestroy)

void FileObjectDestroy(WDFOBJECT wdfFileObject)
{
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_FILEOBJECT, "%!FUNC! Entry");

	PAGED_CODE();

	FileObject* fileObject = GetFileObject(wdfFileObject);

	NT_ASSERT(fileObject != nullptr);

	if (fileObject->m_fileObject == wdfFileObject)
	{
		// File object constructed using placement 'new' so explicitly invoke destructor
		fileObject->~FileObject();
	}
}


FileObject::FileObject(WDFFILEOBJECT wdfFileObject) :
	m_fileObject(wdfFileObject)
{
	NT_ASSERT(m_fileObject != nullptr);
}

FileObject::~FileObject()
{
	m_fileObject = nullptr;
}

Device* FileObject::GetDevice() const
{
	return GetDeviceObject(WdfFileObjectGetDevice(m_fileObject));
}


