// MyCcom.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "Ccom.h"


// 这是已导出类的构造函数。
// 有关类定义的信息，请参阅 Ccom.h
Ccom::Ccom()
{
	_h_com = INVALID_HANDLE_VALUE;
	_is_open = FALSE;
	memset(&_write_overlapped_struct, 0, sizeof(_write_overlapped_struct));
	memset(&_read_overlapped_struct, 0, sizeof(_read_overlapped_struct));
	memset(&_wait_overlapped_struct, 0, sizeof(_wait_overlapped_struct));
}

Ccom::~Ccom()
{

}

BOOL Ccom::Com_Init(pCom_Struct com_struct)
{
	if (_h_com != INVALID_HANDLE_VALUE || _is_open == TRUE)
	{
		Com_Close();
	}
	_h_com = CreateFile(com_struct->port,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		NULL);
	if (_h_com == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	SetupComm(_h_com, com_struct->inqueue, com_struct->outqueue);
	DCB dcb;
	BOOL ret;
	ret = GetCommState(_h_com, &dcb);
	dcb.DCBlength = sizeof(dcb);
	dcb.Parity = com_struct->parity;
	dcb.BaudRate = com_struct->baud;
	dcb.StopBits = com_struct->stopbit;
	dcb.ByteSize = com_struct->bytesize;
	dcb.fDtrControl = 1;
	dcb.fRtsControl = com_struct->fRtsControl;
	dcb.fInX = com_struct->fInX;
	dcb.fOutX = com_struct->fOutX;
	dcb.fOutxCtsFlow = com_struct->fOutxCtsFlow;
	dcb.fTXContinueOnXoff = 1;
	ret = SetCommState(_h_com, &dcb);
	int a = GetLastError();
	PurgeComm(_h_com, PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR | PURGE_TXABORT);
	COMMTIMEOUTS ct;
	ct.ReadIntervalTimeout = MAXWORD;
	ct.ReadTotalTimeoutConstant = 0;
	ct.ReadTotalTimeoutMultiplier = 0;
	ct.WriteTotalTimeoutConstant = 5000;
	ct.WriteTotalTimeoutMultiplier = 500;
	SetCommTimeouts(_h_com, &ct);
	_read_overlapped_struct.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	_write_overlapped_struct.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	_wait_overlapped_struct.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	SetCommMask(_h_com, EV_ERR | EV_RXCHAR);
	_is_open = TRUE;

	return TRUE;
}

DWORD Ccom::Com_Write(LPCTSTR src)
{
	DWORD write_size = 0;
	BOOL ret = FALSE;
	CString str = src;
	DWORD write_len = str.GetLength();

	PurgeComm(_h_com, PURGE_TXABORT | PURGE_TXCLEAR);
	_write_overlapped_struct.Offset = 0;
	USES_CONVERSION;
	char *psrc = T2A(src);

	ret = WriteFile(_h_com, psrc, write_len, &write_size, &_write_overlapped_struct);
	write_size = 0;
	if (ret == FALSE && GetLastError() == ERROR_IO_PENDING)
	{
		if (FALSE == GetOverlappedResult(_h_com, &_write_overlapped_struct, &write_size, TRUE))
		{
			return -1;
		}
	}

	return write_size;
}

DWORD Ccom::Com_Read(LPSTR dst, DWORD len)
{
	COMSTAT cs = { 0 };
	DWORD error = 0;
	DWORD read_size = 0;
	DWORD wait_event = 0;
	BOOL ret = FALSE;
	DWORD read_len = 0;

	_wait_overlapped_struct.Offset = 0;
	ret = WaitCommEvent(_h_com, &wait_event, &_wait_overlapped_struct);
	if (ret == FALSE && GetLastError() == ERROR_IO_PENDING)
	{
		ret = GetOverlappedResult(_h_com, &_wait_overlapped_struct, &read_size, TRUE);
	}

	ClearCommError(_h_com, &error, &cs);
	if (ret == TRUE && wait_event & EV_RXCHAR && cs.cbInQue > 0)
	{
		read_size = 0;
		if (cs.cbInQue > len)
		{
			read_len = len;
		}
		else
		{
			read_len = cs.cbInQue;
		}
		_read_overlapped_struct.Offset = 0;
		ret = ReadFile(_h_com, dst, read_len, &read_size, &_read_overlapped_struct);
		if (ret == FALSE)
		{
			return -1;
		}
		//		PurgeComm(_h_com, PURGE_RXABORT | PURGE_RXCLEAR);
	}

	return read_size;
}

void Ccom::Com_Close(void)
{
	if (_h_com != INVALID_HANDLE_VALUE)
	{
		CloseHandle(_h_com);
		_h_com = INVALID_HANDLE_VALUE;
	}

	if (_write_overlapped_struct.hEvent != NULL)
	{
		CloseHandle(_write_overlapped_struct.hEvent);
		_write_overlapped_struct.hEvent = NULL;
	}
	if (_read_overlapped_struct.hEvent != NULL)
	{
		CloseHandle(_read_overlapped_struct.hEvent);
		_read_overlapped_struct.hEvent = NULL;
	}
	if (_wait_overlapped_struct.hEvent != NULL)
	{
		CloseHandle(_wait_overlapped_struct.hEvent);
		_wait_overlapped_struct.hEvent = NULL;
	}

	_is_open = FALSE;

}

BOOL Ccom::Com_Is_open(void)
{
	return _is_open;
}


#include <objbase.h> 
#include <initguid.h> 
#include <Setupapi.h> 
#pragma comment(lib, "SetupAPI.lib")

// The following define is from ntddser.h in the DDK. It is also 
// needed for serial port enumeration. 
#ifndef GUID_CLASS_COMPORT 
DEFINE_GUID(GUID_CLASS_COMPORT, 0x86e0d1e0L, 0x8089, 0x11d0, 0x9c, 0xe4, \
	0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73);
#endif 

//--------------------------------------------------------------- 
// Helpers for enumerating the available serial ports. 
// These throw a CString on failure, describing the nature of 
// the error that occurred. 

void EnumPortsWdm(vector<SSerInfo> &asi);
void EnumPortsWNt4(vector<SSerInfo> &asi);
void EnumPortsW9x(vector<SSerInfo> &asi);
void SearchPnpKeyW9x(HKEY hkPnp, BOOL bUsbDevice,
	vector<SSerInfo> &asi);


//--------------------------------------------------------------- 
// Routine for enumerating the available serial ports. 
// Throws a CString on failure, describing the error that 
// occurred. If bIgnoreBusyPorts is TRUE, ports that can't 
// be opened for read/write access are not included. 

DLL_API void EnumSerialPorts(vector<SSerInfo> &asi, BOOL bIgnoreBusyPorts)
{
	// Clear the output array 
	//	asi.RemoveAll();

	// Use different techniques to enumerate the available serial 
	// ports, depending on the OS we're using 
	OSVERSIONINFO vi;
	vi.dwOSVersionInfoSize = sizeof(vi);
	if (!::GetVersionEx(&vi)) {
		CString str;
		str.Format(L"Could not get OS version. (err=%lx)",
			GetLastError());
		throw str;
	}
	// Handle windows 9x and NT4 specially 
	if (vi.dwMajorVersion < 5) {
		if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT)
			EnumPortsWNt4(asi);

		else
			EnumPortsW9x(asi);

	}
	else {
		// Win2k and later support a standard API for 
		// enumerating hardware devices. 
		EnumPortsWdm(asi);
	}
	for (int ii = 0; ii < asi.size(); ii++)
	{
		SSerInfo& rsi = asi[ii];
		if (bIgnoreBusyPorts) {
			// Only display ports that can be opened for read/write 
			HANDLE hCom = CreateFile(rsi.strDevPath,
				GENERIC_READ | GENERIC_WRITE,
				0,    /* comm devices must be opened w/exclusive-access */
				NULL, /* no security attrs */
				OPEN_EXISTING, /* comm devices must use OPEN_EXISTING */
				0,    /* not overlapped I/O */
				NULL  /* hTemplate must be NULL for comm devices */
				);
			if (hCom == INVALID_HANDLE_VALUE) {
				// It can't be opened; remove it. 
				asi.erase(asi.begin() + ii);
				ii--;
				continue;
			}
			else {
				// It can be opened! Close it and add it to the list 
				::CloseHandle(hCom);
			}
		}

		// Come up with a name for the device. 
		// If there is no friendly name, use the port name. 
		if (rsi.strFriendlyName.IsEmpty())
			rsi.strFriendlyName = rsi.strPortName;

		// If there is no description, try to make one up from 
		// the friendly name. 
		if (rsi.strPortDesc.IsEmpty()) {
			// If the port name is of the form "ACME Port (COM3)" 
			// then strip off the " (COM3)" 
			rsi.strPortDesc = rsi.strFriendlyName;
			int startdex = rsi.strPortDesc.Find(L" (");
			int enddex = rsi.strPortDesc.Find(L")");
			if (startdex > 0 && enddex ==
				(rsi.strPortDesc.GetLength() - 1))
				rsi.strPortDesc = rsi.strPortDesc.Left(startdex);
		}

	}
}

// Helpers for EnumSerialPorts 

void EnumPortsWdm(vector<SSerInfo> &asi)
{
	CString strErr;
	// Create a device information set that will be the container for  
	// the device interfaces. 
	GUID *guidDev = (GUID*)&GUID_CLASS_COMPORT;

	HDEVINFO hDevInfo = INVALID_HANDLE_VALUE;
	SP_DEVICE_INTERFACE_DETAIL_DATA *pDetData = NULL;

	try {
		hDevInfo = SetupDiGetClassDevs(guidDev,
			NULL,
			NULL,
			DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
			);

		if (hDevInfo == INVALID_HANDLE_VALUE)
		{
			strErr.Format(L"SetupDiGetClassDevs failed. (err=%lx)",
				GetLastError());
			throw strErr;
		}

		// Enumerate the serial ports 
		BOOL bOk = TRUE;
		SP_DEVICE_INTERFACE_DATA ifcData;
		DWORD dwDetDataSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA)+256;
		pDetData = (SP_DEVICE_INTERFACE_DETAIL_DATA*) new char[dwDetDataSize];
		// This is required, according to the documentation. Yes, 
		// it's weird. 
		ifcData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		pDetData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		for (DWORD ii = 0; bOk; ii++) {
			bOk = SetupDiEnumDeviceInterfaces(hDevInfo,
				NULL, guidDev, ii, &ifcData);
			if (bOk) {
				// Got a device. Get the details. 
				SP_DEVINFO_DATA devdata = { sizeof(SP_DEVINFO_DATA) };
				bOk = SetupDiGetDeviceInterfaceDetail(hDevInfo,
					&ifcData, pDetData, dwDetDataSize, NULL, &devdata);
				if (bOk) {
					CString strDevPath(pDetData->DevicePath);
					// Got a path to the device. Try to get some more info. 
					TCHAR fname[256];
					TCHAR desc[256];
					BOOL bSuccess = SetupDiGetDeviceRegistryProperty(
						hDevInfo, &devdata, SPDRP_FRIENDLYNAME, NULL,
						(PBYTE)fname, sizeof(fname), NULL);
					bSuccess = bSuccess && SetupDiGetDeviceRegistryProperty(
						hDevInfo, &devdata, SPDRP_DEVICEDESC, NULL,
						(PBYTE)desc, sizeof(desc), NULL);
					BOOL bUsbDevice = FALSE;
					TCHAR locinfo[256];
					if (SetupDiGetDeviceRegistryProperty(
						hDevInfo, &devdata, SPDRP_LOCATION_INFORMATION, NULL,
						(PBYTE)locinfo, sizeof(locinfo), NULL))
					{
						// Just check the first three characters to determine 
						// if the port is connected to the USB bus. This isn't 
						// an infallible method; it would be better to use the 
						// BUS GUID. Currently, Windows doesn't let you query 
						// that though (SPDRP_BUSTYPEGUID seems to exist in 
						// documentation only). 
						bUsbDevice = (strncmp((char *)locinfo, "USB", 3) == 0);
					}
					if (bSuccess) {
						// Add an entry to the array 
						SSerInfo si;
						si.strDevPath = strDevPath;
						si.strFriendlyName = fname;
						si.strPortDesc = desc;
						si.bUsbDevice = bUsbDevice;
						asi.push_back(si);
					}

				}
				else {
					strErr.Format(L"SetupDiGetDeviceInterfaceDetail failed. (err=%lx)",
						GetLastError());
					throw strErr;
				}
			}
			else {
				DWORD err = GetLastError();
				if (err != ERROR_NO_MORE_ITEMS) {
					strErr.Format(L"SetupDiEnumDeviceInterfaces failed. (err=%lx)", err);
					throw strErr;
				}
			}
		}
	}
	catch (CString strCatchErr) {
		strErr = strCatchErr;
	}

	if (pDetData != NULL)
		delete[](char*)pDetData;
	if (hDevInfo != INVALID_HANDLE_VALUE)
		SetupDiDestroyDeviceInfoList(hDevInfo);

	if (!strErr.IsEmpty())
		throw strErr;
}

void EnumPortsWNt4(vector<SSerInfo> &asi)
{
	// NT4's driver model is totally different, and not that 
	// many people use NT4 anymore. Just try all the COM ports 
	// between 1 and 16 
	SSerInfo si;
	for (int ii = 1; ii <= 16; ii++) {
		CString strPort;
		strPort.Format(L"COM%d", ii);
		si.strDevPath = CString("\\\\.\\") + strPort;
		si.strPortName = strPort;
		asi.push_back(si);
	}
}

void EnumPortsW9x(vector<SSerInfo> &asi)
{
	HKEY hkEnum = NULL;
	HKEY hkSubEnum = NULL;
	HKEY hkSubSubEnum = NULL;

	try {
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Enum", 0, KEY_READ,
			&hkEnum) != ERROR_SUCCESS)
			throw CString("Could not read from HKLM\\Enum");

		// Enumerate the subkeys of HKLM\Enum 
		char acSubEnum[128];
		DWORD dwSubEnumIndex = 0;
		DWORD dwSize = sizeof(acSubEnum);
		while (RegEnumKeyEx(hkEnum, dwSubEnumIndex++, (LPWSTR)acSubEnum, &dwSize,
			NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			HKEY hkSubEnum = NULL;
			if (RegOpenKeyEx(hkEnum, (LPWSTR)acSubEnum, 0, KEY_READ,
				&hkSubEnum) != ERROR_SUCCESS)
				throw CString("Could not read from HKLM\\Enum\\") + (CString)acSubEnum;

			// Enumerate the subkeys of HKLM\Enum\*\, looking for keys 
			// named *PNP0500 and *PNP0501 (or anything in USBPORTS) 
			BOOL bUsbDevice = (strcmp(acSubEnum, "USBPORTS") == 0);
			char acSubSubEnum[128];
			dwSize = sizeof(acSubSubEnum);  // set the buffer size 
			DWORD dwSubSubEnumIndex = 0;
			while (RegEnumKeyEx(hkSubEnum, dwSubSubEnumIndex++, (LPWSTR)acSubSubEnum,
				&dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
			{
				BOOL bMatch = (strcmp(acSubSubEnum, "*PNP0500") == 0 ||
					strcmp(acSubSubEnum, "*PNP0501") == 0 ||
					bUsbDevice);
				if (bMatch) {
					HKEY hkSubSubEnum = NULL;
					if (RegOpenKeyEx(hkSubEnum, (LPWSTR)acSubSubEnum, 0, KEY_READ,
						&hkSubSubEnum) != ERROR_SUCCESS)
						throw CString("Could not read from HKLM\\Enum\\") +
						(CString)acSubEnum + L"\\" + (CString)acSubSubEnum;
					SearchPnpKeyW9x(hkSubSubEnum, bUsbDevice, asi);
					RegCloseKey(hkSubSubEnum);
					hkSubSubEnum = NULL;
				}

				dwSize = sizeof(acSubSubEnum);  // restore the buffer size 
			}

			RegCloseKey(hkSubEnum);
			hkSubEnum = NULL;
			dwSize = sizeof(acSubEnum); // restore the buffer size 
		}
	}
	catch (CString strError) {
		if (hkEnum != NULL)
			RegCloseKey(hkEnum);
		if (hkSubEnum != NULL)
			RegCloseKey(hkSubEnum);
		if (hkSubSubEnum != NULL)
			RegCloseKey(hkSubSubEnum);
		throw strError;
	}

	RegCloseKey(hkEnum);
}

void SearchPnpKeyW9x(HKEY hkPnp, BOOL bUsbDevice,
	vector<SSerInfo> &asi)
{
	HKEY hkSubPnp = NULL;

	try {
		// Enumerate the subkeys of HKLM\Enum\*\PNP050[01] 
		char acSubPnp[128];
		DWORD dwSubPnpIndex = 0;
		DWORD dwSize = sizeof(acSubPnp);
		while (RegEnumKeyEx(hkPnp, dwSubPnpIndex++, (LPWSTR)acSubPnp, &dwSize,
			NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
		{
			HKEY hkSubPnp = NULL;
			if (RegOpenKeyEx(hkPnp, (LPWSTR)acSubPnp, 0, KEY_READ,
				&hkSubPnp) != ERROR_SUCCESS)
				throw CString("Could not read from HKLM\\Enum\\...\\")
				+ (CString)acSubPnp;

			// Look for the PORTNAME value 
			char acValue[128];
			dwSize = sizeof(acValue);
			if (RegQueryValueEx(hkSubPnp, L"PORTNAME", NULL, NULL, (BYTE*)acValue,
				&dwSize) == ERROR_SUCCESS)
			{
				CString strPortName(acValue);

				// Got the portname value. Look for a friendly name. 
				CString strFriendlyName;
				dwSize = sizeof(acValue);
				if (RegQueryValueEx(hkSubPnp, L"FRIENDLYNAME", NULL, NULL, (BYTE*)acValue,
					&dwSize) == ERROR_SUCCESS)
					strFriendlyName = acValue;

				// Prepare an entry for the output array. 
				SSerInfo si;
				si.strDevPath = CString("\\\\.\\") + strPortName;
				si.strPortName = strPortName;
				si.strFriendlyName = strFriendlyName;
				si.bUsbDevice = bUsbDevice;

				// Overwrite duplicates. 
				BOOL bDup = FALSE;
				for (int ii = 0; ii<asi.size() && !bDup; ii++)
				{
					if (asi[ii].strPortName == strPortName) {
						bDup = TRUE;
						asi[ii] = si;
					}
				}
				if (!bDup) {
					// Add an entry to the array 
					asi.push_back(si);
				}
			}

			RegCloseKey(hkSubPnp);
			hkSubPnp = NULL;
			dwSize = sizeof(acSubPnp);  // restore the buffer size 
		}
	}
	catch (CString strError) {
		if (hkSubPnp != NULL)
			RegCloseKey(hkSubPnp);
		throw strError;
	}
}
