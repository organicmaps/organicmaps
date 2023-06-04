/*
	100% free public domain implementation of the HMAC-SHA1 algorithm
	by Chien-Chung, Chung (Jim Chung) <jimchung1221@gmail.com>
*/


#ifndef __HMAC_SHA1_H__
#define __HMAC_SHA1_H__

#include "SHA1.h"

typedef unsigned char BYTE ;

class CHMAC_SHA1 : public CSHA1
{
public:

    enum {
        SHA1_DIGEST_LENGTH	= 20,
        SHA1_BLOCK_SIZE		= 64
    } ;

private:
    BYTE m_ipad[SHA1_BLOCK_SIZE];
    BYTE m_opad[SHA1_BLOCK_SIZE];

    // This holds one SHA1 block's worth of data, zero padded if necessary.
    char SHA1_Key[SHA1_BLOCK_SIZE];

public:
    CHMAC_SHA1() {}

    void HMAC_SHA1(BYTE *text, int text_len, BYTE *key, int key_len, BYTE *digest);
};


#endif /* __HMAC_SHA1_H__ */
