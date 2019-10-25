#include "globals.h"
using namespace Proxy;
#pragma comment(lib, "ws2_32.lib")

#define DATA_BUFSIZE 8192
#define PORT 15400 //15400 V15  5400 ECLIPSE //19291 Insanity //5800 Crystal
FILE *pFile = nullptr;
SOCKET monsocketSend;
SOCKET monsocketRcv;
SOCKET monsocketCompletionStatut;
DWORD count;
LPDWORD lpNumberOfBytesRCV;
LPDWORD flags;
LPWSAOVERLAPPED overlap;
LPWSAOVERLAPPED_COMPLETION_ROUTINE Compl;

DWORD m_dwIndex(0);
DWORD m_dwOffset(13); // 9 POUR LA V19 13 POUR V15
DWORD m_dwSize(0);
//Resend and Hash stuff
WSABUF buf[128];
BOOL CRC_Valid = false;
DWORD m_dwJoinLen;
DWORD m_dwJoinRes;
DWORD dataHash;
CRC32 m_Crc;

BYTE ci_pbData[65536]; //FOR CI

//PBUYING_INFO2 CI;

BYTE  m_pbData[1024];

DWORD m_dwSessionId;
DWORD HashKeyRaito;
CRITICAL_SECTION g_mHashKey;


bool SessionId = false;
char WorldSessionID[4];

std::string string_to_hex(const std::string& input)
{
	static const char* const lut = "0123456789ABCDEF";
	size_t len = input.length();
	std::string output;
	output.reserve(2 * len);
	for (size_t i = 0; i < len; ++i)
	{
		const unsigned char c = input[i];
		output.push_back(lut[c >> 4]);
		output.push_back(lut[c & 15]);
	}
	return output;
}

template <class type>
void Append(type data)
{
	if (m_dwIndex < m_dwOffset)
	{
		m_dwIndex = m_dwOffset;
		m_dwSize = m_dwOffset;
	}

	size_t totalSize = sizeof(type);
	memcpy(ci_pbData + m_dwIndex, &data, sizeof(type));
	m_dwIndex += (int)totalSize;
	m_dwSize += (int)totalSize;
}


bool StringToHex(const std::string &inStr, unsigned char *outStr)
{
	size_t len = inStr.length();
	for (size_t i = 0; i < len; i += 2) {
		sscanf(inStr.c_str() + i, "%2hhx", outStr);
		++outStr;
	}
	return true;
}


std::string hex_to_string(std::string& input)
{
	static const char* const lut = "0123456789ABCDEF";
	size_t len = input.length();
	if (len & 1) throw std::invalid_argument("odd length");

	std::string output;
	output.reserve(len / 2);
	for (size_t i = 0; i < len; i += 2)
	{
		char a = input[i];
		const char* p = std::lower_bound(lut, lut + 16, a);
		if (*p != a) throw std::invalid_argument("not a hex digit");

		char b = input[i + 1];
		const char* q = std::lower_bound(lut, lut + 16, b);
		if (*q != b) throw std::invalid_argument("not a hex digit");

		output.push_back(((p - lut) << 4) | (q - lut));
	}
	return output;
}


int (WINAPI *pWSASend)(SOCKET, LPWSABUF, DWORD, LPDWORD, DWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE) = WSASend;
int  WINAPI MyWSASend(SOCKET, LPWSABUF, DWORD, LPDWORD, DWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);

int (WINAPI *pWSARecv)(SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE) = WSARecv;
int  WINAPI MyWSARecv(SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);

BOOL(WINAPI *pGetQueuedCompletionStatus) (HANDLE, LPDWORD, PULONG_PTR, LPOVERLAPPED *, DWORD) = GetQueuedCompletionStatus;
BOOL WINAPI MyGetQueuedCompletionStatus(HANDLE, LPDWORD, PULONG_PTR, LPOVERLAPPED *, DWORD);

BOOL(WSAAPI *pWSAGetOverlappedResult)(SOCKET s, LPWSAOVERLAPPED lpOverlapped, LPDWORD lpcbTransfer, BOOL fWait, LPDWORD lpdwFlags) = WSAGetOverlappedResult;
BOOL WSAAPI MyWSAGetOverlappedResult(SOCKET s, LPWSAOVERLAPPED lpOverlapped, LPDWORD lpcbTransfer, BOOL fWait, LPDWORD lpdwFlags);


int WINAPI MyWSASend(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	struct sockaddr_in sinn;
	int len = sizeof(sinn);

	if (getpeername(s, (struct sockaddr*)&sinn, &len) != -1)
	{
		std::cout << "Packet SEND : " << "Port number: " << int(ntohs(sinn.sin_port)) << std::endl;
	}

	if (ntohs(sinn.sin_port) == PORT)
	{
		monsocketSend = (SOCKET)s;
	}

	MyForm::richTextBox1->AppendText("PACKET SEND: ");
	std::cout << "Data: ";
	for (int i = 0; i < int(lpBuffers->len); i++)
	{
		char buffer;
		buffer = lpBuffers->buf[i];
		std::string test(1, buffer);
		std::string Hextest = string_to_hex(test);
		if (i == 1 || i == 5 || i == 9 || i == 13 || i == 17 || i == 21)
		{
			std::cout << "   ";
		}
		std::cout << Hextest;
		MyForm::Send = gcnew String(Hextest.c_str());
		MyForm::richTextBox1->AppendText(MyForm::Send);
	}
	std::cout << std::endl;

	MyForm::richTextBox1->AppendText(" \n");
	MyForm::richTextBox1->AppendText(" \n");
	MyForm::richTextBox1->ScrollToCaret();
	
	return pWSASend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, NULL, lpOverlapped, NULL);
}

int  WINAPI MyWSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRCV, LPDWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	// Reference the WSAOVERLAPPED structure as a SOCKET_INFORMATION structure

	int rv = -1;
	char hookmsg[DATA_BUFSIZE];
	wchar_t dbgmsg[sizeof(hookmsg) * 2];

		rv = pWSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRCV, dwFlags, lpOverlapped, lpCompletionRoutine);


		monsocketRcv = s;

		for (int i = 0; i < dwBufferCount; i++)
		{			
			buf[0].len = lpBuffers[i].len;
			buf[0].buf = lpBuffers[i].buf;
		}
		
		overlap = lpOverlapped;
	return rv;
}


int MyForm::FunctionMyReSend(String^ Param )
{
	
	//UI string to std string convertion
	std::string MySendd = msclr::interop::marshal_as<std::string>(MyForm::richTextBox2->Text);
	MySendd.erase(std::remove_if(MySendd.begin(), MySendd.end(), ::isspace), MySendd.end());
	std::string StringMySend = hex_to_string(MySendd);
	std::cout << StringMySend << "lenght: " << StringMySend.length() << std::endl;
	DWORD len = StringMySend.length();


	//initialize CRC crack

	BYTE b_dwjoinLen[4];  //Will contain DWORD dwDjoinLen (longueur data)
	BYTE b_dwJoinRes[4];  //m_dwJoinRes(hash longueur data)
	BYTE b_dataHash[4];   //DataLenghtHash
	BYTE SendData[128];

	unsigned char outStr[65536]; //Will contain our Hex value of the textbox
	StringToHex(MySendd, outStr); //Convert textbox String into Hexvalue


	for (int k = 0; k < 4; k++)
	{
		b_dwjoinLen[k] = outStr[k + 5];
		b_dwJoinRes[k] = outStr[k + 1];
		b_dataHash[k] = outStr[k + 9];
		//std::cout << std::hex << b_dataHash[k];
	}

	WORD x_Len = MAKEWORD(b_dwjoinLen[2], b_dwjoinLen[3]);
	WORD y_Len = MAKEWORD(b_dwjoinLen[0], b_dwjoinLen[1]);
	m_dwJoinLen = MAKELONG(y_Len, x_Len);

	WORD x_Res = MAKEWORD(b_dwJoinRes[2], b_dwJoinRes[3]);
	WORD y_Res = MAKEWORD(b_dwJoinRes[0], b_dwJoinRes[1]);
	m_dwJoinRes = MAKELONG(y_Res, x_Res);

	WORD x_Data = MAKEWORD(b_dataHash[2], b_dataHash[3]);
	WORD y_Data = MAKEWORD(b_dataHash[0], b_dataHash[1]);
	dataHash = MAKELONG(y_Data, x_Data);

	std::cout << "Check du DWORD Lenght packet : " << std::hex << m_dwJoinLen << std::endl;
	std::cout << "Check du DWORD HashLenght packet : " << std::hex << m_dwJoinRes << std::endl;
	std::cout << "Check du DWORD HashData packet : " << std::hex << dataHash << std::endl;

	std::cout << std::endl;

	const char *Textbox = StringMySend.c_str();
	std::cout << "Mon content en charactere ascii :";
	int a;
	for (a = 0; a < len; a++)
	{
		std::cout << Textbox[a];
		SendData[a] = Textbox[a];
	}

	std::cout << std::endl;
	printf("Sending:\n");
	HexDumpData(SendData, StringMySend.length());
	WSABUF WSABuf = { 0 };

	WSABuf.buf = (char *)SendData;
	WSABuf.len = len;
	OVERLAPPED* pOverlapped = NULL;
	DWORD LengthSent = 0;

	int reSend = pWSASend(monsocketSend, &WSABuf, 1, &LengthSent, NULL, NULL, NULL);
	return 0;
}


int MyForm::FunctionMyFakeSend(String^ Param)
{
	
	//CUSTOM V15	



	std::string MySendCopie = msclr::interop::marshal_as<std::string>(MyForm::richTextBox2->Text);
	MySendCopie.erase(std::remove_if(MySendCopie.begin(), MySendCopie.end(), ::isspace), MySendCopie.end());
	unsigned char outStr[65536]; //Will contain our Hex value of the textbox
	StringToHex(MySendCopie, outStr); //Convert textbox String into Hexvalue

	
	//CRC
//	MyForm::bruteforceCrcKey();
	DWORD dwDataSize = (MySendCopie.length() / 2) - 13;
	//std::cout << "Data Size :" << dwDataSize << std::endl;
	DWORD m_dwSize = MySendCopie.length() / 2;
	//NOW BRUTEFORCE CRC FIRST!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


	//BYTE  m_pbData[1024];
	memset(&m_pbData, 0, 1024);
	m_pbData[0] = 0x5e;
	DWORD toWrite = 0;
	BYTE digest[4];
	char str[4];
	BYTE *ptr = outStr + 13;
	DWORD Weird = 0xFFFFFFFF;

	DWORD rest = 0;
	BYTE restData[1024];
	memset(&restData, 0, 1024);

	memcpy(str, &m_dwSize, sizeof(DWORD));
	WSABUF WSABuf = { 0 };
	WSABuf.len = m_dwSize;
	OVERLAPPED* pOverlapped = NULL;
	DWORD LengthSent = 0;

	// Hash from m_dwSize
	m_Crc.Restart();
	m_Crc.Update((const byte*)(&dwDataSize), sizeof(DWORD));
	m_Crc.Final(digest, HashKeyRaito);

	//Swap dword before memcpy
	toWrite = ~(*(UNALIGNED DWORD*)digest ^ m_dwSessionId);
	//toWrite = _byteswap_ulong(toWrite);
//	std::cout << "Hash from m_dwsize digested :" << toWrite << std::endl;


	memcpy(m_pbData + 1, &toWrite, sizeof(DWORD));

	//DWORD SizePacket = _byteswap_ulong(dwDataSize);
	// m_dwSize
	memcpy(m_pbData + 5, &dwDataSize, sizeof(DWORD));
//	std::cout << "Digested Data size :" << dwDataSize << std::endl;

	// Hash from Data
	m_Crc.Restart();
	m_Crc.Update((const byte*)(ptr), dwDataSize);
	m_Crc.Final(digest, HashKeyRaito);


	toWrite = ~(*(UNALIGNED DWORD*)digest ^ m_dwSessionId);
	//toWrite = _byteswap_ulong(toWrite);
	memcpy(m_pbData + 9, &toWrite, sizeof(DWORD));


//	std::cout << "Digested Data Hash :" << toWrite << std::endl;

	memcpy(m_pbData + 13, &Weird, sizeof(DWORD));

	
//	std::cout << "rest of data ";
	for (int x = 0; x < dwDataSize-4; x++)
	{
		memcpy(m_pbData +17+x, &outStr[17 + x], sizeof(BYTE));
		//std::cout << m_pbData + 17 + x;
		//WSABuf.buf[x+13] = outStr[x + 13];
		//std::cout  << restData[x];
	}

	WSABuf.buf = (char *)m_pbData;

	std::cout << "FAKE SEND :";
	for (int q = 0; q < WSABuf.len ; q++)
	{
		char buffer;
		buffer = WSABuf.buf[q];
		std::string test(1, buffer);
		std::string Hextest = string_to_hex(test);
		if (q == 1 || q == 5 || q == 9 || q == 13 || q == 17 || q == 21)
		{
			std::cout << "  ";
		}
		std::cout << Hextest;
		//std::cout << std::hex << WSABuf.buf[q];
	}
	int sendcustom = pWSASend(monsocketSend, &WSABuf, 1, &LengthSent, NULL, NULL, NULL);

	std::cout << std::endl;
	
	return 0;
	
	
	/*
	// CUSTOM V19
	
	std::string MySendCopie = msclr::interop::marshal_as<std::string>(MyForm::richTextBox2->Text);
	MySendCopie.erase(std::remove_if(MySendCopie.begin(), MySendCopie.end(), ::isspace), MySendCopie.end());
	unsigned char outStr[65536]; //Will contain our Hex value of the textbox
	StringToHex(MySendCopie, outStr); //Convert textbox String into Hexvalue

	DWORD dwDataSize = (MySendCopie.length() / 2) - 9;
	DWORD m_dwSize = MySendCopie.length() / 2;

	memset(&m_pbData, 0, 1024);
	m_pbData[0] = 0x5e;
	DWORD toWrite = 0;
	BYTE digest[4];
	char str[4];
	BYTE *ptr = outStr + 13;
	DWORD Weird = 0xFFFFFFFF;

	DWORD rest = 0;
	BYTE restData[1024];
	memset(&restData, 0, 1024);

	memcpy(str, &m_dwSize, sizeof(DWORD));
	WSABUF WSABuf = { 0 };
	WSABuf.len = m_dwSize;
	OVERLAPPED* pOverlapped = NULL;
	DWORD LengthSent = 0;


	memcpy(m_pbData + 1, &dwDataSize, sizeof(DWORD));
	DWORD uncrypted = 0x00000000;
	memcpy(m_pbData + 5, &uncrypted, sizeof(DWORD));
	std::cout << "Digested Data size :" << dwDataSize << std::endl;

	DWORD max = 0xFFFFFFFF;
	memcpy(m_pbData + 9, &max, sizeof(DWORD));

	std::cout << "rest of data ";
	for (int x = 0; x < dwDataSize - 4; x++)
	{
		memcpy(m_pbData + 13 + x, &outStr[13 + x], sizeof(BYTE));
		std::cout << m_pbData + 13 + x;
		//WSABuf.buf[x+13] = outStr[x + 13];
		//std::cout  << restData[x];
	}

	WSABuf.buf = (char *)m_pbData;

	std::cout << "Total buffer send :";
	for (int q = 0; q < WSABuf.len; q++)
	{
		std::cout << WSABuf.buf[q];
	}
	int sendcustom = pWSASend(monsocketSend, &WSABuf, 1, &LengthSent, NULL, NULL, NULL);

	std::cout << std::endl;
	return 0;
	
	*/
}

//HACK
void client_sendBuyingInfo(PRaito3 pBuyingInfo)
{	
//CUSTOM V15

	Append(DWORD(0xFFFFFFFF));
	Append(DWORD(PACKETTYPE_BUYING_INFO));
	Append((Raito3)*pBuyingInfo);

	ci_pbData[0] = 0x5E;
	DWORD dwDataSize = m_dwSize - m_dwOffset;
	DWORD toWrite = 0;
	BYTE digest[4];
	char str[4];
	BYTE *ptr = ci_pbData + m_dwOffset;

	memcpy(str, &m_dwSize, sizeof(DWORD));



	// Hash from m_dwSize
	m_Crc.Restart();
	m_Crc.Update((const byte*)(&dwDataSize), sizeof(DWORD));
	m_Crc.Final(digest, HashKeyRaito);

	toWrite = ~(*(UNALIGNED DWORD*)digest ^ m_dwSessionId);
	memcpy(ci_pbData + 1, &toWrite, sizeof(DWORD));

	// m_dwSize
	memcpy(ci_pbData + 5, &dwDataSize, sizeof(DWORD));

	// Hash from Data
	m_Crc.Restart();
	m_Crc.Update((const byte*)(ptr), dwDataSize);
	m_Crc.Final(digest, HashKeyRaito);

	toWrite = ~(*(UNALIGNED DWORD*)digest ^m_dwSessionId);
	memcpy(ci_pbData + 9, &toWrite, sizeof(DWORD));

	std::cout <<"Size data :" << m_dwSize << std::endl;
	std::cout << "Contenu du packet Create Perrin";

	for (int i = 0; i < m_dwSize; i++)
	{
		//std::cout << ci_pbData[i];
		std::string test(1, ci_pbData[i]);
		std::string Hextest = string_to_hex(test);
		std::cout << Hextest;
	}
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;
	WSABUF buf = { 0 };
	buf.len = m_dwSize;
	buf.buf = (char *)ci_pbData;
	DWORD LengthSent = 0;


	m_dwIndex = 0;
	m_dwSize = 0;
	int sendcustom = pWSASend(monsocketSend, &buf, 1, &LengthSent, NULL, NULL, NULL);

	
	/*
	//FLyff V19

	Append(DWORD(0xFFFFFFFF));
	Append(DWORD(PACKETTYPE_BUYING_INFO));
	Append((Raito2)*pBuyingInfo);


	ci_pbData[0] = 0x00;
	DWORD dwDataSize = m_dwSize - m_dwOffset;
	DWORD toWrite = 0;
	BYTE digest[4];
	char str[4];
	BYTE *ptr = ci_pbData + m_dwOffset;

	memcpy(str, &m_dwSize, sizeof(DWORD));




	memcpy(ci_pbData + 1, &dwDataSize, sizeof(DWORD));

	DWORD uncrypted = 0x00000000;
	memcpy(ci_pbData + 5, &uncrypted, sizeof(DWORD));

	std::cout << "Contenu du packet Create Perrin";

	std::cout << "Packet lenght " << m_dwSize << std::endl;
	std::cout << "Contenu du packet Create Perrin";
	for (int i = 0; i < m_dwSize; i++)
	{
		//std::cout << ci_pbData[i];
		std::string test(1, ci_pbData[i]);
		std::string Hextest = string_to_hex(test);
		std::cout << Hextest;
	}
	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;
	WSABUF buf = { 0 };
	buf.len = m_dwSize;
	buf.buf = (char *)ci_pbData;
	DWORD LengthSent = 0;


	m_dwIndex = 0;
	m_dwSize = 0;


	int sendcustom = pWSASend(monsocketSend, &buf, 1, &LengthSent, NULL, NULL, NULL);
	*/
	
}

//HACK
void MyForm::CreateItem()
{
	Raito3 CI;

	std::string ID = msclr::interop::marshal_as<std::string>(MyForm::textBox1->Text);
	ID.erase(std::remove_if(ID.begin(), ID.end(), ::isspace), ID.end());
	int numberID;
	std::istringstream issID(ID);
	issID >> CI.dwItemId;


	std::string NUM = msclr::interop::marshal_as<std::string>(MyForm::textBox2->Text);
	NUM.erase(std::remove_if(NUM.begin(), NUM.end(), ::isspace), NUM.end());
	int numberNUM;
	std::istringstream issNUM(NUM);
	issNUM >> CI.dwItemNum;


	client_sendBuyingInfo(&CI);

}

//HACK
bool validateHash()
{
	// screw it :(
	if (m_dwJoinLen == 0)
		return false;

	// CALC HASH with OWN key
	DWORD dwOwnHash = 0;
	BYTE digest[4];

	// Hash from c_Size
	m_Crc.Restart();
	m_Crc.Update((const byte*)(&m_dwJoinLen), sizeof(DWORD));
	m_Crc.Final(digest, HashKeyRaito); //g_pconn->get_Hashkey = set_HashKey param or loop param

	dwOwnHash = ~(*(UNALIGNED DWORD*)digest ^ m_dwSessionId);

	// Compare ?!?
	if (dwOwnHash == m_dwJoinRes)
		return true;

	return false;
}

//HACK
bool set_HashKey(DWORD dwHashKey)
{
	HashKeyRaito = dwHashKey;

	CRC_Valid = validateHash();

	return CRC_Valid;
}

//HACK
DWORD WINAPI MyForm::bruteforceCrcKey()
{
	for (unsigned int i = 0xFFFFFFFF; i >= 0x00000000; i--)
	{
		/*if (g_bBruteForcingCRC == false)
		{
			__LOG(C_LIGHTBLUE, "INFO", "CRC bruteforce aborted.");
			return NULL;
		}
		*/
		std::cout << "Key test =" << i << std::endl;
		if (set_HashKey(i))
		{
			std::cout << "CRC Key cracked :" << HashKeyRaito; std::cout << std::endl;
			//__LOG(C_GREEN, "SUCCESS", "Bruteforced CRC key: 0x%08X", g_pConn->get_HashKey());

			//g_bBruteForcingCRC = false;
			return NULL;
		}
	}

	//g_bBruteForcingCRC = false;

	return 0;
}


BOOL WINAPI MyGetQueuedCompletionStatus(HANDLE CompletionPort, LPDWORD lpNumberOfBytesTransferred, PULONG_PTR lpCompletionKey, LPOVERLAPPED * lpOverlapped, DWORD dwMilliseconds)
{
	BOOL a = pGetQueuedCompletionStatus(CompletionPort, lpNumberOfBytesTransferred, lpCompletionKey, lpOverlapped, dwMilliseconds);

	struct sockaddr_in sinn;
	int len = sizeof(sinn);

	getpeername(monsocketRcv, (struct sockaddr*)&sinn, &len);
	 
	if (*lpOverlapped == overlap && overlap!= NULL)
	{
		if (SessionId == false)
		{
			if (ntohs(sinn.sin_port) == PORT)
			{
				std::cout << "Session ID: ";
				SessionId = true;
				int b;
				BYTE testID[4];
				for (b = 0; b < 4; b++)
				{
					
					// b+13 for Eclipse (v19 prob)
					// b+9 for v15
					//IDSession[b] = buf->buf[b + 9]; // SessionID save
					testID[b] = buf->buf[b + 9];
					//std::cout << std::hex << testID[b];
					//m_dwSessionId = buf->buf[b + 9];
					std::string test(1, testID[b]);
					std::string Hextest = string_to_hex(test);
					std::cout << Hextest;					
					//WorldSessionID[b] = buf->buf[b + 9];
					
				}
				

				//Calcule Session ID in DWORD for Hash;
				WORD y = MAKEWORD(testID[2], testID[3]);
				WORD x = MAKEWORD(testID[0], testID[1]);
				m_dwSessionId = MAKELONG(x, y);
				//m_dwSessionId = std::strtoul(IDSession.c_str(), NULL, 16);
				
				//DWORD test = 0x50;
				//m_dwSessionId = std::strtoul(IDSession.c_str(), NULL, 0);
				//std::cout << "  Session ID check again :" << std::hex << m_dwSessionId;
				std::cout << std::endl;
			}
		}
		
		int i;
		std::cout << "Packet RCV: ";
		for (int i = 0; i < *lpNumberOfBytesTransferred; i++)
		{
			char bufferr;
			bufferr = buf->buf[i];
			std::string test(1, bufferr);
			std::string Hextest = string_to_hex(test);
			std::cout << Hextest;
		}
		//std::cout << "CompletionStatus Packet RCV from port :" << int(ntohs(sinn.sin_port)) << "  Number of bytes :" << *lpNumberOfBytesTransferred << " Content :";
		std::cout << std::endl;
		
	}

	overlap = NULL;
	
	return a;
}

void MyForm::Hook()
{

	AllocConsole();
	freopen_s(&pFile, "CONOUT$", "w", stdout);


	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)pWSASend, MyWSASend);
	DetourAttach(&(PVOID&)pWSARecv, MyWSARecv);
	DetourAttach(&(PVOID&)pGetQueuedCompletionStatus, MyGetQueuedCompletionStatus);
	DetourTransactionCommit();
}






