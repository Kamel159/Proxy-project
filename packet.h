#pragma once

#include "globals.h"

class CPacket {
public:
	CPacket();
	CPacket(BYTE bType);
	~CPacket();


	int get_Offset() { return m_dwIndex; }

	void SetIndex(int index);
	void Skip(int nNum);
	void WriteHeader();

	void GetStr(BYTE *str);
	void SetSize(int size);
	int GetSize();

	template <class type>
	void Read(type& buffer, bool advancePosition = true)
	{
		size_t totalSize = sizeof(type);

		memcpy(&buffer, m_pbData + m_dwIndex, totalSize);

		if (advancePosition)
			m_dwIndex += totalSize;
	}


	template <class type>
	void ReadArray(type* buffer, int size, bool advancePosition = true)
	{
		size_t totalSize = sizeof(type) * size;

		memcpy(buffer, m_pbData + m_dwIndex, totalSize);

		if (advancePosition)
			m_dwIndex += totalSize;
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
		memcpy(m_pbData + m_dwIndex, &data, sizeof(type));
		m_dwIndex += (int)totalSize;
		m_dwSize += (int)totalSize;
	}

	template <class type>
	void AppendArray(type& data, size_t count)
	{
		if (m_dwIndex < m_dwOffset)
		{
			m_dwIndex = m_dwOffset;
			m_dwSize = m_dwOffset;
		}

		size_t totalSize = sizeof(data[0]) * count;
		memcpy(m_pbData + m_dwIndex, data, totalSize);
		m_dwIndex += (int)totalSize;
		m_dwSize += (int)totalSize;
	}

	template <class type>
	void Fill(type data, int count)
	{
		if (m_dwIndex < m_dwOffset)
		{
			m_dwIndex = m_dwOffset;
			m_dwSize = m_dwOffset;
		}

		size_t totalSize = sizeof(data) * count;

		for (int i = 0; i < count; i++)
		{
			memcpy(m_pbData + m_dwIndex, &data, sizeof(type));
			m_dwIndex += (int)sizeof(type);
		}

		m_dwSize += (int)totalSize;
	}

	template <class type>
	void Initialize(type& data, size_t count)
	{
		m_dwIndex = 0;
		m_dwSize = 0;
		memset(&m_pbData, 0, 65536);
		size_t totalSize = sizeof(data[0]) * count;
		memcpy(m_pbData + m_dwIndex, data, totalSize);
		m_dwSize += (int)totalSize;
	}

private:
	BYTE   m_pbData[65536];
	DWORD  m_dwIndex;
	DWORD  m_dwSize;

	BYTE  m_bType;
	DWORD m_dwOffset;

	CRC32 m_Crc;
};
