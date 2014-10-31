//-----------------------------------------------------------------------------
//
//                                  swSSO
//
//       SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
//
//                Copyright (C) 2004-2014 - Sylvain WERDEFROY
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
// swSSOAdmin.cpp
//-----------------------------------------------------------------------------
// Fonctions sp�cifiques au mode de fonctionnement admin
//-----------------------------------------------------------------------------

#include "stdafx.h"
static int giRefreshTimer=10;

//-----------------------------------------------------------------------------
// CheckAdminPwd()
//-----------------------------------------------------------------------------
// V�rifie le mot de passe admin en base
//-----------------------------------------------------------------------------
int CheckAdminPwd(char *szPwd)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	UNREFERENCED_PARAMETER(szPwd);


	rc=0;
//end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// StoreAdminPwd()
//-----------------------------------------------------------------------------
// Stocke le mot de passe admin en base
//-----------------------------------------------------------------------------
int StoreAdminPwd(char *szNewPwd)
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	
	char szRequest[512+1];
	char *pszResult=NULL;
	char szHash[2*32+1];

	swCryptEncodeBase64((const unsigned char*)gcszK1,8,szHash);
	swCryptEncodeBase64((const unsigned char*)gcszK2,8,szHash+16);
	swCryptEncodeBase64((const unsigned char*)gcszK3,8,szHash+32);
	swCryptEncodeBase64((const unsigned char*)gcszK4,8,szHash+48);
	TRACE_BUFFER((TRACE_DEBUG,_F_,(unsigned char*)szHash,strlen(szHash),"szHash :"));
	sprintf_s(szRequest,sizeof(szRequest),"%s?action=defineadminpwd&hash=%s&pwd=%s",gszWebServiceAddress,szHash,szNewPwd);
	pszResult=HTTPRequest(szRequest,8,NULL);
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szRequest)); goto end; }
	rc=(strcmp(pszResult,"OK")==0)?0:-1;

end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// AskAdminPwdDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la fen�tre de saisie du mot de passe admin
//-----------------------------------------------------------------------------
static int CALLBACK AskAdminPwdDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);

	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// init champ de saisie
			SendMessage(GetDlgItem(w,TB_NEW_PWD1),EM_LIMITTEXT,LEN_PWD,0);
			SendMessage(GetDlgItem(w,TB_NEW_PWD2),EM_LIMITTEXT,LEN_PWD,0);
			// titre en gras
			SetTextBold(w,TX_FRAME);
			MACRO_SET_SEPARATOR;
			// magouille supr�me : pour g�rer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui ex�cutera un invalidaterect pour forcer la peinture
			if (giRefreshTimer==giTimer) giRefreshTimer=11;
			SetTimer(w,giRefreshTimer,200,NULL);
			break;
		case WM_TIMER:
			TRACE((TRACE_INFO,_F_,"WM_TIMER (refresh)"));
			if (giRefreshTimer==(int)wp) 
			{
				KillTimer(w,giRefreshTimer);
				InvalidateRect(w,NULL,FALSE);
				SetForegroundWindow(w); 
			}
			break;
		case WM_CTLCOLORSTATIC:
			int ctrlID;
			ctrlID=GetDlgCtrlID((HWND)lp);
			switch(ctrlID)
			{
				case TX_FRAME:
					SetBkMode((HDC)wp,TRANSPARENT);
					rc=(int)GetStockObject(HOLLOW_BRUSH);
					break;
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDOK:
				{
					char szPwd[LEN_PWD+1];
					int ret;
					GetDlgItemText(w,TB_NEW_PWD1,szPwd,sizeof(szPwd));
					ret=CheckAdminPwd(szPwd);
					SecureZeroMemory(szPwd,strlen(szPwd));
					if (ret==0)
						EndDialog(w,IDOK);
					else
						MessageBox(w,GetString(IDS_BADPWD),"swSSO",MB_ICONEXCLAMATION);
					break;
				}
				case IDCANCEL:
					EndDialog(w,IDCANCEL);
					break;
				case TB_PWD:
					if (HIWORD(wp)==EN_CHANGE)
					{
						char szPwd[LEN_PWD+1];
						int len;
						len=GetDlgItemText(w,TB_PWD,szPwd,sizeof(szPwd));
						EnableWindow(GetDlgItem(w,IDOK),len==0 ? FALSE : TRUE);
					}
					break;
			}
			break;
		case WM_HELP:
			Help();
			break;
		case WM_PAINT:
			DrawLogoBar(w,50,ghLogoFondBlanc50);
			rc=TRUE;
			break;
	}
	return rc;
}

//-----------------------------------------------------------------------------
// AskAdminPwd() 
//-----------------------------------------------------------------------------
// Demande le mot de passe admin et le v�rifie
//-----------------------------------------------------------------------------
int AskAdminPwd()
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;

	if (DialogBox(ghInstance,MAKEINTRESOURCE(IDD_PWD_ADMIN_ASK),NULL,AskAdminPwdDialogProc)==IDOK) rc=0;

//end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}

//-----------------------------------------------------------------------------
// DefineAdminPwdDialogProc()
//-----------------------------------------------------------------------------
// DialogProc de la fen�tre de d�finition du mot de passe admin
//-----------------------------------------------------------------------------
static int CALLBACK DefineAdminPwdDialogProc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	UNREFERENCED_PARAMETER(lp);

	int rc=FALSE;
	switch (msg)
	{
		case WM_INITDIALOG:
			TRACE((TRACE_DEBUG,_F_, "WM_INITDIALOG"));
			SendMessage(w,WM_SETICON,ICON_BIG,(LPARAM)ghIconAltTab);
			SendMessage(w,WM_SETICON,ICON_SMALL,(LPARAM)ghIconSystrayActive); 
			// init champ de saisie
			SendMessage(GetDlgItem(w,TB_NEW_PWD1),EM_LIMITTEXT,LEN_PWD,0);
			SendMessage(GetDlgItem(w,TB_NEW_PWD2),EM_LIMITTEXT,LEN_PWD,0);
			// titre en gras
			SetTextBold(w,TX_FRAME);
			MACRO_SET_SEPARATOR;
			// magouille supr�me : pour g�rer les cas rares dans lesquels la peinture du bandeau & logo se fait mal
			// on active un timer d'une seconde qui ex�cutera un invalidaterect pour forcer la peinture
			if (giRefreshTimer==giTimer) giRefreshTimer=11;
			SetTimer(w,giRefreshTimer,200,NULL);
			break;
		case WM_TIMER:
			TRACE((TRACE_INFO,_F_,"WM_TIMER (refresh)"));
			if (giRefreshTimer==(int)wp) 
			{
				KillTimer(w,giRefreshTimer);
				InvalidateRect(w,NULL,FALSE);
				SetForegroundWindow(w); 
			}
			break;
		case WM_CTLCOLORSTATIC:
			int ctrlID;
			ctrlID=GetDlgCtrlID((HWND)lp);
			switch(ctrlID)
			{
				case TX_FRAME:
					SetBkMode((HDC)wp,TRANSPARENT);
					rc=(int)GetStockObject(HOLLOW_BRUSH);
					break;
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDOK:
				{
					char szNewPwd1[LEN_PWD+1];
					int ret;
					GetDlgItemText(w,TB_NEW_PWD1,szNewPwd1,sizeof(szNewPwd1));
					ret=StoreAdminPwd(szNewPwd1);
					SecureZeroMemory(szNewPwd1,strlen(szNewPwd1));
					if (ret==0)
						EndDialog(w,IDOK);
					else
						MessageBox(w,GetString(IDS_ADMIN_PWD_STORE_ERROR),"swSSO",MB_ICONEXCLAMATION);
					break;
				}
				case IDCANCEL:
					EndDialog(w,IDCANCEL);
					break;
				case TB_NEW_PWD1:
				case TB_NEW_PWD2:
				{
					char szNewPwd1[LEN_PWD+1];
					char szNewPwd2[LEN_PWD+1];
					int lenNewPwd1,lenNewPwd2;
					if (HIWORD(wp)==EN_CHANGE)
					{
						BOOL bOK;
						lenNewPwd1=GetDlgItemText(w,TB_NEW_PWD1,szNewPwd1,sizeof(szNewPwd1));
						lenNewPwd2=GetDlgItemText(w,TB_NEW_PWD2,szNewPwd2,sizeof(szNewPwd2));
						bOK=FALSE;
						if (lenNewPwd1!=0 && lenNewPwd1==lenNewPwd2)
						{
							bOK=((strcmp(szNewPwd1,szNewPwd2)==0) ? TRUE : FALSE);
						}
						EnableWindow(GetDlgItem(w,IDOK),bOK);
					}
					break;
				}
			}
			break;
		case WM_HELP:
			Help();
			break;
		case WM_PAINT:
			DrawLogoBar(w,50,ghLogoFondBlanc50);
			rc=TRUE;
			break;
	}
	return rc;
}

//-----------------------------------------------------------------------------
// DefineAdminPwd() 
//-----------------------------------------------------------------------------
// Demande de d�finir le mot de passe admin et l'enregistre en base
//-----------------------------------------------------------------------------
int DefineAdminPwd()
{
	TRACE((TRACE_ENTER,_F_, ""));
	int rc=-1;
	
	if (DialogBox(ghInstance,MAKEINTRESOURCE(IDD_PWD_ADMIN_DEFINE),NULL,DefineAdminPwdDialogProc)==IDOK) rc=0;
	
//end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}


//-----------------------------------------------------------------------------
// IsAdminPwdDefined() 
//-----------------------------------------------------------------------------
// Regarde en base si le mot de passe admin est d�fini
//-----------------------------------------------------------------------------
BOOL IsAdminPwdDefined()
{
	TRACE((TRACE_ENTER,_F_, ""));
	BOOL rc=FALSE;
	char szRequest[512+1];
	char *pszResult=NULL;

	sprintf_s(szRequest,sizeof(szRequest),"%s?action=isadminpwd",gszWebServiceAddress);
	pszResult=HTTPRequest(szRequest,8,NULL);
	if (pszResult==NULL) { TRACE((TRACE_ERROR,_F_,"HTTPRequest(%s)=NULL",szRequest)); goto end; }
	
	rc=(strcmp(pszResult,"YES")==0);

end:
	TRACE((TRACE_LEAVE,_F_, "rc=%d",rc));
	return rc;
}