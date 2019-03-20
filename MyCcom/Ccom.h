// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 MYCCOM_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// MYCCOM_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef DLL_EXPORTS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

#pragma once

#include <atlstr.h>
#include <iostream>
#include <vector>

using namespace std;


typedef struct _Com_Struct
{
	_Com_Struct() : port(L"COM1"), baud(115200UL), inqueue(2048),
					outqueue(2048), parity(NOPARITY), stopbit(ONESTOPBIT),
					bytesize(8), fOutxCtsFlow(0), fOutX(0), fInX(0),
					fRtsControl(1)
					{}
	CString port;
	DWORD baud;
	DWORD inqueue;
	DWORD outqueue;
	BYTE parity;
	BYTE stopbit;
	BYTE bytesize;
	BYTE fOutxCtsFlow : 1;
	BYTE fOutX : 1;
	BYTE fInX : 1;
	BYTE fRtsControl : 2;
	BYTE fres : 3;
}Com_Struct, *pCom_Struct;

// 此类是从 Ccom.dll 导出的
class DLL_API Ccom 
{
private:

	HANDLE _h_com;
	volatile BOOL _is_open;
	OVERLAPPED _write_overlapped_struct;
	OVERLAPPED _read_overlapped_struct;
	OVERLAPPED _wait_overlapped_struct;

public:

	Ccom();
	virtual ~Ccom();
	BOOL Com_Is_open(void);
	virtual BOOL Com_Init(pCom_Struct com_struct);
	void Com_Close(void);
	virtual DWORD Com_Write(LPCTSTR src);
	virtual DWORD Com_Read(LPSTR dst, DWORD len);
};
 
struct SSerInfo {
	SSerInfo() : bUsbDevice(FALSE) {}
	CString strDevPath;          // Device path for use with CreateFile() 
	CString strPortName;         // Simple name (i.e. COM1) 
	CString strFriendlyName;     // Full name to be displayed to a user 
	BOOL bUsbDevice;             // Provided through a USB connection? 
	CString strPortDesc;         // friendly name without the COMx 
};


// Routine for enumerating the available serial ports. Throws a CString on 
// failure, describing the error that occurred. If bIgnoreBusyPorts is TRUE, 
// ports that can't be opened for read/write access are not included. 
DLL_API void EnumSerialPorts(vector<SSerInfo> &asi, BOOL bIgnoreBusyPorts = TRUE);