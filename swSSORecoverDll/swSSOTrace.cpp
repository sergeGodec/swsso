//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                C�opyright (C) 2004-2013 - Sylvain WERDEFROY
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
// swSSOTrace.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"

#define REGKEY_TRACE			"SOFTWARE\\swSSO\\TraceRecoverDll"
#define REGVALUE_TRACE_LEVEL	"Level" 
#define REGVALUE_TRACE_FILENAME "FileName"
#define REGVALUE_TRACE_FILESIZE "FileSize"

static char gszTraceFileName[260+1];
static int giTraceLevel=TRACE_ERROR;
static DWORD gdwTraceFileSize=20000000; 
static char gszTraceBuf[2048];
HANDLE ghfTrace=INVALID_HANDLE_VALUE;

static char gszTraceLevelLabel[5+1];
HANDLE ghTraceMutex=NULL;

//-----------------------------------------------------------------------------
// swTraceOpen()
//-----------------------------------------------------------------------------
// Lecture de la configuration des traces et ouverture du fichier trace
// ISSUE#277 : la configuration ds traces est d�sormais lue dans le .ini 
//             et les traces sont produites m�me par la version release
//-----------------------------------------------------------------------------
void swTraceOpen(void)
{
	DWORD lenConfigFile;
	char szConfigFile[1024];

	DWORD dwWaitForSingleObject;
	ghTraceMutex=CreateMutex(NULL,TRUE,"Global\\swSSORecoverDll.traces");
	if (ghTraceMutex==NULL) goto end;
	if (GetLastError()==ERROR_ALREADY_EXISTS)
	{
		dwWaitForSingleObject=WaitForSingleObject(ghTraceMutex,2000);
		if (dwWaitForSingleObject!=WAIT_OBJECT_0) goto end;
	}

	// reconstitue le chemin complet du fichier de configuration
	lenConfigFile=GetModuleFileName(ghModule,szConfigFile,sizeof(szConfigFile));
	if (lenConfigFile<10) goto end;
	memcpy(szConfigFile+lenConfigFile-3,"ini",3);

	// lit les infos du keystore dans le fichier de configuration
	GetPrivateProfileString("Logs","Filename","",gszTraceFileName,sizeof(gszTraceFileName),szConfigFile);
	if (*gszTraceFileName==0) goto end; // nom de fichier pas trouv�, pas de traces
	gdwTraceFileSize=GetPrivateProfileInt("Logs","Filesize",20,szConfigFile)*1000000;
	giTraceLevel=GetPrivateProfileInt("Logs","Level",1,szConfigFile);
	if (giTraceLevel>=TRACE_PWD) giTraceLevel=TRACE_DEBUG;
	
	// ouverture du fichier (ferm� uniquement sur appel de swTraceClose)
	ghfTrace=CreateFile(gszTraceFileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	// si fichier existe, se positionne � la fin du fichier pour �critures ult�rieures
	if (ghfTrace!=INVALID_HANDLE_VALUE) SetFilePointer(ghfTrace,0,0,FILE_END);

end:;
}

//-----------------------------------------------------------------------------
// swTraceClose()
//-----------------------------------------------------------------------------
// Est-il vraiment n�cessaire de commenter la fonction de cette fonction ? ;-)
//-----------------------------------------------------------------------------
void swTraceClose(void)
{
	if (ghfTrace==INVALID_HANDLE_VALUE) goto end;
	CloseHandle(ghfTrace); 
	ghfTrace=INVALID_HANDLE_VALUE; 
	if (ghTraceMutex!=NULL)
	{
		ReleaseMutex(ghTraceMutex);
		CloseHandle(ghTraceMutex);
		ghTraceMutex=NULL;
	}
end:;
}

//-----------------------------------------------------------------------------
// swGetTraceLevelLabel()
//-----------------------------------------------------------------------------
static char *swGetTraceLevelLabel(int iLevel)
{
	switch (iLevel)
	{
		case TRACE_ERROR: strcpy_s(gszTraceLevelLabel,sizeof(gszTraceLevelLabel),"ERROR"); break;
		case TRACE_ENTER: strcpy_s(gszTraceLevelLabel,sizeof(gszTraceLevelLabel),"  -> "); break;
		case TRACE_LEAVE: strcpy_s(gszTraceLevelLabel,sizeof(gszTraceLevelLabel)," <-  "); break;
		case TRACE_INFO:  strcpy_s(gszTraceLevelLabel,sizeof(gszTraceLevelLabel),"INFO "); break;
		case TRACE_DEBUG: strcpy_s(gszTraceLevelLabel,sizeof(gszTraceLevelLabel),"DEBUG"); break;
		case TRACE_PWD:	  strcpy_s(gszTraceLevelLabel,sizeof(gszTraceLevelLabel),"*PWD*"); break;
	}
	return gszTraceLevelLabel;
}
//-----------------------------------------------------------------------------
// swTraceWrite()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void swTraceWrite(int iLevel,char *szFunction,char *szTrace, ...)
{
	int len;
	SYSTEMTIME horodate;
	char *psz=gszTraceBuf;
	DWORD dw;

	if (ghfTrace==INVALID_HANDLE_VALUE) goto end;
	if (iLevel>giTraceLevel) goto end;

	// fait tourner le fichier si n�cessaire
	if (GetFileSize(ghfTrace,NULL)>gdwTraceFileSize)
	{
		SetFilePointer(ghfTrace,0,NULL,FILE_BEGIN);
		SetEndOfFile(ghfTrace);
	}

	// en-t�te : horodate + niveau + nom de la fonction
	GetLocalTime(&horodate);
	len=wsprintf(psz,"%02d/%02d-%02d:%02d:%02d:%03d %s %s ",
		(int)horodate.wDay,(int)horodate.wMonth,
		(int)horodate.wHour,(int)horodate.wMinute,(int)horodate.wSecond,(int)horodate.wMilliseconds,
		swGetTraceLevelLabel(iLevel),szFunction);
	// trace
	len+=wvsprintf(gszTraceBuf+len,szTrace,(char *)(&szTrace+1));
	// retour chariot
	memcpy(gszTraceBuf+len,"\r\n\0",3);
	len+=2;
	WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
	FlushFileBuffers(ghfTrace);
end:;
}

//-----------------------------------------------------------------------------
// swTraceWriteBuffer()
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void swTraceWriteBuffer(int iLevel,char *szFunction,unsigned char *pBuffer,int lenBuffer,const char *szTrace, ...)
{
	int len;
	int i,iBinOffset,iCharOffset;
	SYSTEMTIME horodate;
	char *psz=gszTraceBuf;
	DWORD dw;

	if (ghfTrace==INVALID_HANDLE_VALUE) goto end;
	if (iLevel>giTraceLevel) goto end;

	// fait tourner le fichier si n�cessaire
	if (GetFileSize(ghfTrace,NULL)>gdwTraceFileSize)
	{
		SetFilePointer(ghfTrace,0,NULL,FILE_BEGIN);
		SetEndOfFile(ghfTrace);
	}

	// en-t�te : horodate + niveau + nom de la fonction
	GetLocalTime(&horodate);
	len=wsprintf(psz,"%02d/%02d-%02d:%02d:%02d:%03d %s %s ",
		(int)horodate.wDay,(int)horodate.wMonth,
		(int)horodate.wHour,(int)horodate.wMinute,(int)horodate.wSecond,(int)horodate.wMilliseconds,
		swGetTraceLevelLabel(iLevel),szFunction);
	// trace
	len+=wvsprintf(gszTraceBuf+len,szTrace,(char *)(&szTrace+1));
	// retour chariot
	memcpy(gszTraceBuf+len,"\r\n\0",3);
	len+=2;
	WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
	// v�rif buffer pas NULL
	if (pBuffer==NULL) 
	{
		strcpy_s(gszTraceBuf,sizeof(gszTraceBuf),"NULL\r\n");
		len=(int)strlen(gszTraceBuf);
		WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
		goto end;
	}
	// trace de la longueur
	len=wsprintf(gszTraceBuf,"lenBuffer=%d\r\n",lenBuffer);
	WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
	// pr�formatage
	strcpy_s(gszTraceBuf,sizeof(gszTraceBuf),"                                                |                 \r\n");
	len=(int)strlen(gszTraceBuf);
	// trace le contenu du buffer
	iBinOffset=0;
	iCharOffset=50;
	for (i=0;i<lenBuffer;i++)
	{
		if (i!=0 && (i%16)==0)
		{ 
			WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
			iBinOffset=0;
			iCharOffset=50;
			memset(gszTraceBuf,' ',len-2);
		}
		wsprintf(gszTraceBuf+iBinOffset,"%02x",(unsigned char)(pBuffer[i]));
		iBinOffset+=2;
		gszTraceBuf[iBinOffset]=' '; // a �t� �cras� par le 0 du wsprintf
		if (pBuffer[i]>=32 && pBuffer[i]<=127)
			gszTraceBuf[iCharOffset]=pBuffer[i];
		else
			gszTraceBuf[iCharOffset]='.';
		iBinOffset++;
		iCharOffset++;
	}
	//if ((i%16)!=0) 0.93B6 : visiblement empechait de voir la derni�re ligne des buffers multiples de 16 octets !
	{ 
		WriteFile(ghfTrace,gszTraceBuf,len,&dw,NULL);
	}
	FlushFileBuffers(ghfTrace);
end:;
}
