#pragma once

#include <Windows.h>
#include "crc.h"
#include <assert.h>

#define NULL 0

typedef unsigned int word32;
typedef unsigned char byte;

#define IS_LITTLE_ENDIAN 

#define CRC32_INDEX(c) (c & 0xff)
#define CRC32_SHIFTED(c) (c >> 8)

//! CRC Checksum Calculation
class CRC32
{
public:
	enum { DIGESTSIZE = 4 };
	CRC32();
	void Restart();
	void Final(byte *digest, DWORD dwHashKey) { TruncatedFinal(digest, DigestSize(), dwHashKey); }

	void Update(const byte *input, unsigned int length);
	void TruncatedFinal(byte *hash, unsigned int size, DWORD dwHashKey);

	unsigned int DigestSize() const { return DIGESTSIZE; }

	void UpdateByte(byte b) { m_crc = m_tab[CRC32_INDEX(m_crc) ^ b] ^ CRC32_SHIFTED(m_crc); }
	byte GetCrcByte(unsigned int i) const { return ((byte *)&(m_crc))[i]; }


private:
	inline void Reset();
	inline void ThrowIfInvalidTruncatedSize(unsigned int size) const;

	static const word32 m_tab[256];
	word32 m_crc;
};


