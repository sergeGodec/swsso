//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2016 - Sylvain WERDEFROY
//
//							 http://www.swsso.fr
//                   
//                             sylvain@swsso.fr
//
//-----------------------------------------------------------------------------
// 
//  This file is part of swSSO.
//  
//  swSSO is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  swSSO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with swSSO.  If not, see <http://www.gnu.org/licenses/>.
// 
//-----------------------------------------------------------------------------
// swSSOCrypt.h
//-----------------------------------------------------------------------------

#define SALT_LEN 128		// longueur du sel appliqu� au mdp maitre avant hash (versions < 0.93)
#define HASH_LEN 20			// SHA1=160 bits
#define PBKDF2_SALT_LEN	64	// longueur du sel utilis� avec PBKDF2 (512 bits)
#define PBKDF2_PWD_LEN 32	// 256 bits
#define AES256_KEY_LEN 32   // 256 bits
#define AES256_KEY_PART_LEN 8

#define PWDTYPE_ALPHA			1
#define PWDTYPE_NUM				2
#define PWDTYPE_SPECIALCHARS	4

#pragma warning(disable:4200) 
typedef struct
{
    BLOBHEADER header;
    DWORD dwKeySize;
	BYTE KeyData[]; // g�n�re le warning C4200
} KEYBLOB;
#pragma warning(default:4200)

extern HCRYPTPROV ghProv;

int  swCryptInit();
void swCryptTerm();
int  swCryptSaltAndHashPassword(char *bufSalt, const char *szPwd,char **pszHashedPwd, int iNbIterations,bool bV72);
void swCryptEncodeBase64(const unsigned char *pSrcData,int lenSrcData,char *pszDestString);
int swCryptDecodeBase64(const char *szSrcString,char *pDestData,int lenDestData);

int swPBKDF2(BYTE *bufResult,int bufResultLen,const char *szPwd,const BYTE *bufSalt,int bufSaltLen,int iNbIterations);

BOOL swIsPBKDF2KeySaltReady(void);
BOOL swIsPBKDF2PwdSaltReady(void);
int swGenPBKDF2Salt(void);
int swReadPBKDF2Salt(void);

int  swCryptEncryptData(unsigned char *iv,unsigned char *pData,DWORD lData,int iKeyId);
int  swCryptDecryptDataAES256(unsigned char *iv, unsigned char *pData,DWORD lData,int iKeyId);

int swStoreAESKey(BYTE *AESKeyData,int iKeyId);
int swCryptDeriveKey(const char *pszMasterPwd,int iKeyId);
char *swCryptEncryptString(const char *pszSource,int iKeyId);
char *swCryptDecryptString(const char *pszSource,int iKeyId);

void swGenerateRandomPwd(char *pszPwd,int iPwdLen,int iPwdType);
