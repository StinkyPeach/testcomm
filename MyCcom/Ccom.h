// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� MYCCOM_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// MYCCOM_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
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

// �����Ǵ� Ccom.dll ������
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