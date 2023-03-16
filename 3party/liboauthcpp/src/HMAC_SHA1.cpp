//******************************************************************************
//* HMAC_SHA1.cpp : Implementation of HMAC SHA1 algorithm
//*                 Comfort to RFC 2104
//*
//******************************************************************************
#include "HMAC_SHA1.h"
#include <iostream>
#include <memory>


void CHMAC_SHA1::HMAC_SHA1(BYTE *text, int text_len, BYTE *key, int key_len, BYTE *digest)
{
	memset(SHA1_Key, 0, SHA1_BLOCK_SIZE);

	/* repeated 64 times for values in ipad and opad */
	memset(m_ipad, 0x36, sizeof(m_ipad));
	memset(m_opad, 0x5c, sizeof(m_opad));

	/* STEP 1 */
	if (key_len > SHA1_BLOCK_SIZE)
	{
		CSHA1::Reset();
		CSHA1::Update((UINT_8 *)key, key_len);
		CSHA1::Final();

		CSHA1::GetHash((UINT_8 *)SHA1_Key);
	}
	else
		memcpy(SHA1_Key, key, key_len);

	/* STEP 2 */
	for (int i=0; i<(int)sizeof(m_ipad); i++)
	{
		m_ipad[i] ^= SHA1_Key[i];
	}

	/* STEP 4 */
	CSHA1::Reset();
        CSHA1::Update((UINT_8 *)m_ipad, sizeof(m_ipad));
        CSHA1::Update((UINT_8 *)text, text_len);
	CSHA1::Final();

        char szReport[SHA1_DIGEST_LENGTH];
	CSHA1::GetHash((UINT_8 *)szReport);

	/* STEP 5 */
	for (int j=0; j<(int)sizeof(m_opad); j++)
	{
		m_opad[j] ^= SHA1_Key[j];
	}

	/*STEP 7 */
	CSHA1::Reset();
        CSHA1::Update((UINT_8 *)m_opad, sizeof(m_opad));
        CSHA1::Update((UINT_8 *)szReport, SHA1_DIGEST_LENGTH);
	CSHA1::Final();

	CSHA1::GetHash((UINT_8 *)digest);
}
