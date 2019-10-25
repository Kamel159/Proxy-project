#pragma once

#include "MyForm.h"
#include <WinSock2.h>
#include <Windows.h>
#include <string>
#include "detours.h"
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <msclr\marshal_cppstd.h>
#include "MySocket.h"
#include "HexDumpData.h"
#include <vector>
#include "ptypes.h"
#include "crc.h"
#include "globals.h"
#include <cstdlib>
#include "packet.h"
#include <intrin.h>
#include <TlHelp32.h>

//Data structure of an exploitable packet
typedef struct _Raito
{
	DWORD dwServerIndex;
	DWORD dwPlayerId;
	DWORD dwSenderId;
	DWORD dwItemId;
	DWORD dwItemNum;
	char  szBxaid[21]; // bxaid
	DWORD dwRetVal;

	//ALL VALUES DOES NOT MATTER BUT ID/NUM
	_Raito()
	{
		dwServerIndex = 0;
		dwPlayerId = 0;
		dwItemId = 0; //Id of the item we want to create
		dwItemNum = 0;//The number of that item
		dwRetVal = 0;
		ZeroMemory(szBxaid, sizeof(char) * 21);
		dwSenderId = 1;
	}
}
Raito, *PRaito;

typedef struct _Raito2 : public _Raito
{
	DWORD dpid;
	DWORD dwKey;

	_Raito2() : _Raito()
	{
		dpid = 0xFFFFFFFF; //Doesnt matter
		dwKey = 0xF;
	}
}
Raito2, *PRaito2;

typedef struct _Raito3 : public _Raito2
{
	DWORD	dwTickCount;
	_Raito3() : _Raito2()
	{
		//			dwTickCount		= GetTickCount();
	}
}
Raito3, *PRaito3;


extern DWORD HashKeyRaito;
extern CRITICAL_SECTION g_mHashKey;
extern DWORD m_dwSessionId;
extern BOOL CRC_Valid;
extern BOOL g_bBruteForcingCRC;
extern CRC32 m_Crc;

extern DWORD m_dwJoinLen;
extern DWORD m_dwJoinRes;
extern DWORD dataHash;

extern DWORD m_dwOffset;
extern DWORD m_dwIndex;
extern DWORD m_dwSize;

extern BYTE m_pbData[];

//extern PBUYING_INFO2 CI;
