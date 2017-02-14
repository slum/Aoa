// autoOA.cpp : �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
//

#include "stdafx.h"
#include "autoOA.h"

#define MAX_LOADSTRING 100
#define INQ_TIMER_ID 1000
#define INQ_TIMER_ID_LAST 1000

// �O���[�o���ϐ�:
HINSTANCE hInst;								// ���݂̃C���^�[�t�F�C�X
HWND m_mainWnd;
TCHAR szTitle[MAX_LOADSTRING];					// �^�C�g�� �o�[�̃e�L�X�g
TCHAR szWindowClass[MAX_LOADSTRING];			// ���C�� �E�B���h�E �N���X��

//LPTSTR m_addr; // �Ј��i���o�[/�L�[ 
//LPTSTR m_urlKey; // �Ј��i���o�[/�L�[ 
UINT m_inqSec;
UINT m_lastHour;

enum TIMER_STATUS {
	FIRST,
	MID,
	LAST
} m_timer_status;

LPCWCHAR m_uid;
LPCWCHAR m_pass;

LPCWCHAR m_attAddr;
LPCWCHAR m_leaveAddr;


// ���̃R�[�h ���W���[���Ɋ܂܂��֐��̐錾��]�����܂�:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
DWORD				ChooseAuthScheme(DWORD);
BOOL				CallHttpRequest(LPCWCHAR);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
void TrayMenu(HWND);
BOOL                Att();
BOOL                Leave();
BOOL Ballon(LPSTR, LPSTR, DWORD, UINT);
void CALLBACK		InternetCallback(HINTERNET, DWORD_PTR, DWORD, LPVOID, DWORD);
void CALLBACK TimerProc(
	HWND hwnd,         // �E�B���h�E�̃n���h��
	UINT uMsg,         // WM_TIMER ���b�Z�[�W
	UINT_PTR idEvent,  // �^�C�}�̎��ʎq
	DWORD dwTime       // ���݂̃V�X�e������
	);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: �����ɃR�[�h��}�����Ă��������B
	MSG msg;
	HACCEL hAccelTable;

	// �O���[�o������������������Ă��܂��B
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_AUTOOA, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	WCHAR uid[100];
	WCHAR upassword[100];
	WCHAR attAddr[100];
	WCHAR leaveAddr[100];
	CHAR inqSec[100];
	CHAR lastHour[100];
	m_uid = uid;
	m_pass = upassword;
	//m_addr = addr;
	//m_urlKey = urlKey;
	m_attAddr = attAddr;
	m_leaveAddr = leaveAddr;

	if (lstrlen(lpCmdLine) == 0) {

		GetPrivateProfileStringW(L"UserData", L"Uid", L"", uid, sizeof(uid), L"ini\\autoOA.ini");
		GetPrivateProfileStringW(L"UserData", L"UPassword", L"", upassword, sizeof(upassword), L"ini\\autoOA.ini");
		//GetPrivateProfileString("UserData", "Addr", "", addr, sizeof(addr), "ini\\autoOA.ini");
		//GetPrivateProfileString("UserData", "Key", "", urlKey, sizeof(urlKey), "ini\\autoOA.ini");
		GetPrivateProfileStringW(L"UserData", L"AttAddr", L"", attAddr, sizeof(attAddr), L"ini\\autoOA.ini");
		GetPrivateProfileStringW(L"UserData", L"LeaveAddr", L"", leaveAddr, sizeof(leaveAddr), L"ini\\autoOA.ini");
		GetPrivateProfileString("Timer", "InqTime", "", inqSec, sizeof(inqSec), "ini\\autoOA.ini");
		GetPrivateProfileString("Timer", "LastHour", "", lastHour, sizeof(lastHour), "ini\\autoOA.ini");
	}
	else {

		WCHAR wcstring[100];
		ZeroMemory(wcstring, sizeof(wcstring));
		size_t convertedChars = 0;
		size_t orgChars = 0;

		mbstowcs_s(&convertedChars, &wcstring[0], lstrlen(lpCmdLine) + 1, lpCmdLine, _TRUNCATE);

		GetPrivateProfileStringW(L"UserData", L"Uid", L"", uid, sizeof(uid), wcstring);
		GetPrivateProfileStringW(L"UserData", L"UPassword", L"", upassword, sizeof(upassword), wcstring);
		//GetPrivateProfileString("UserData", "Key", "", urlKey, sizeof(urlKey), lpCmdLine);
		//GetPrivateProfileString("UserData", "Addr", "", addr, sizeof(addr), lpCmdLine);
		GetPrivateProfileStringW(L"UserData", L"AttAddr", L"", attAddr, sizeof(attAddr), wcstring);
		GetPrivateProfileStringW(L"UserData", L"LeaveAddr", L"", leaveAddr, sizeof(leaveAddr), wcstring);
		GetPrivateProfileString("Timer", "InqTime", "", inqSec, sizeof(inqSec), lpCmdLine);
		GetPrivateProfileString("Timer", "lastHour", "", lastHour, sizeof(lastHour), lpCmdLine);

	}

	// �A�v���P�[�V�����̏����������s���܂�:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_AUTOOA));

	m_inqSec = atoi(inqSec) * 1000;
	m_lastHour = atoi(lastHour);

	m_timer_status = FIRST;
	SetTimer(m_mainWnd, INQ_TIMER_ID, 1000, TimerProc);

	// ���C�� ���b�Z�[�W ���[�v:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

void CALLBACK TimerProc(

	HWND hwnd,         // �E�B���h�E�̃n���h��
	UINT uMsg,         // WM_TIMER ���b�Z�[�W
	UINT_PTR idEvent,  // �^�C�}�̎��ʎq
	DWORD dwTime       // ���݂̃V�X�e������
	) {

	KillTimer(m_mainWnd, INQ_TIMER_ID);

	switch (m_timer_status)
	{
	case FIRST:
		break;
	case MID:
		Leave();
		break;
	case LAST:
		Leave();
		break;
	default:
		break;
	}

	// �^�C�}�[�ݒ肵�Ȃ��Ɨ��p���Ȃ�
	if (m_inqSec != 0) {

		// �Œ�10����
		if (m_inqSec < 600 * 1000) {
			m_inqSec = 600 * 1000;
		}

		SYSTEMTIME time;
		GetLocalTime(&time);
		INT w = ((m_lastHour - time.wHour) * 3600 - (time.wMinute * 60) - time.wSecond) * 1000;

		if (w < 0) {
			// ���łɎ��Ԃ��������߂Ȃɂ����Ȃ�
		}
		else if (w > m_inqSec) {
			m_timer_status = MID;
			SetTimer(m_mainWnd, INQ_TIMER_ID, m_inqSec, TimerProc);
		}
		else {
			m_timer_status = LAST;
			SetTimer(m_mainWnd, INQ_TIMER_ID, w, TimerProc);
		}
	}

}


//
//  �֐�: MyRegisterClass()
//
//  �ړI: �E�B���h�E �N���X��o�^���܂��B
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_AUTOOA));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_AUTOOA);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

DWORD ChooseAuthScheme(DWORD dwSupportedSchemes)
{
	//  It is the server's responsibility only to accept 
	//  authentication schemes that provide a sufficient level
	//  of security to protect the server's resources.
	//
	//  The client is also obligated only to use an authentication
	//  scheme that adequately protects its username and password.
	//
	//  Thus, this sample code does not use Basic authentication  
	//  because Basic authentication exposes the client's username 
	//  and password to anyone monitoring the connection.

	if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_NEGOTIATE)
		return WINHTTP_AUTH_SCHEME_NEGOTIATE;
	else if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_NTLM)
		return WINHTTP_AUTH_SCHEME_NTLM;
	else if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_PASSPORT)
		return WINHTTP_AUTH_SCHEME_PASSPORT;
	else if (dwSupportedSchemes & WINHTTP_AUTH_SCHEME_DIGEST)
		return WINHTTP_AUTH_SCHEME_DIGEST;
	else
		return 1;
}


//
//   �֐�: InitInstance(HINSTANCE, int)
//
//   �ړI: �C���X�^���X �n���h����ۑ����āA���C�� �E�B���h�E���쐬���܂��B
//
//   �R�����g:
//
//        ���̊֐��ŁA�O���[�o���ϐ��ŃC���X�^���X �n���h����ۑ����A
//        ���C�� �v���O���� �E�B���h�E���쐬����ѕ\�����܂��B
//
BOOL CallHttpRequest(LPCWCHAR addr)
{
	HINTERNET s_hSession = NULL;
	HINTERNET m_hConnect = NULL;
	HINTERNET m_hRequest = NULL;
	LPWSTR wstrServer;
	INTERNET_PORT nPort = 0;
	LPWSTR wstrVerb;
	LPWSTR wstrObjectName;
	LPWSTR wstrHeaders;

	DWORD dwStatusCode = 0;
	DWORD dwSupportedSchemes;
	DWORD dwFirstScheme;
	DWORD dwSelectedScheme;
	DWORD dwTarget;
	DWORD dwLastStatus = 0;
	DWORD dwSize = sizeof(DWORD);
	BOOL  bResults = FALSE;
	BOOL  bDone = FALSE;

	DWORD dwDownloaded = 0;
	LPSTR pszOutBuffer;

	DWORD dwProxyAuthScheme = 0;


	WINHTTP_CURRENT_USER_IE_PROXY_CONFIG winConfig;
	ZeroMemory(&winConfig, sizeof(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG));
	WinHttpGetIEProxyConfigForCurrentUser(&winConfig);
	if (winConfig.lpszProxy)
	{
		s_hSession = WinHttpOpen(L"Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko",	// UserAgent:IE11
			WINHTTP_ACCESS_TYPE_NAMED_PROXY,
			winConfig.lpszProxy,
			winConfig.lpszProxyBypass ? winConfig.lpszProxyBypass : WINHTTP_NO_PROXY_BYPASS,
			WINHTTP_FLAG_ASYNC);


	}
	else {

		s_hSession = WinHttpOpen(L"Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko",	// UserAgent:IE11
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			WINHTTP_FLAG_ASYNC);

	}

	if (s_hSession != NULL)
	{
		URL_COMPONENTS urlcomponents;
		ZeroMemory(&urlcomponents, sizeof(URL_COMPONENTS));
		urlcomponents.dwStructSize = sizeof(URL_COMPONENTS);
		WCHAR wszHostName[URLBUFFER_SIZE];
		WCHAR wszUrlPath[URLBUFFER_SIZE];
		urlcomponents.lpszHostName = wszHostName;
		urlcomponents.lpszUrlPath = wszUrlPath;
		urlcomponents.dwHostNameLength = URLBUFFER_SIZE;
		urlcomponents.dwUrlPathLength = URLBUFFER_SIZE;
		if (!WinHttpCrackUrl(addr,
			0,
			0,
			&urlcomponents))
		{

			MessageBoxW(NULL, L"URL��͂Ɏ��s", L"URL��͂Ɏ��s", 0);
			return -1;
		}

		wstrServer = urlcomponents.lpszHostName;
		wstrObjectName = urlcomponents.lpszUrlPath;
		nPort = urlcomponents.nPort;

		// HTTP��HTTPS������ȊO��
		DWORD dwOpenRequestFlag = (INTERNET_SCHEME_HTTPS == urlcomponents.nScheme) ? WINHTTP_FLAG_SECURE : 0;

		// POST��GET��
		if (false)
		{	// POST
			wstrVerb = L"POST";
			wstrHeaders = L"Content-Type: application/x-www-form-urlencoded";
			//if (0 != tstrParameter.length())
			//{	// �p�����[�^���A���M����I�v�V�����f�[�^�ɕϊ�����
			//	char* pszOptional = NhT2M(tstrParameter.c_str());	// char������ɕϊ�
			//	strOptional = pszOptional;
			//	free(pszOptional);
			//}
		}
		else
		{	// GET
			wstrVerb = L"GET";
			wstrHeaders = L"";
			//if (0 != tstrParameter.length())
			//{	// �I�u�W�F�N�g�ƃp�����[�^���u?�v�ŘA��
			//	WCHAR* pwszBuffer = NhT2W(tstrParameter.c_str());
			//	wstrObjectName += L"?" + wstring(pwszBuffer);
			//	free(pwszBuffer);
			//}
		}

		// HTTP�ڑ�
		m_hConnect = WinHttpConnect(s_hSession,
			wstrServer,
			nPort,
			0);
		if (NULL == m_hConnect)
		{
			MessageBoxW(NULL, L"HTTP�ڑ��Ɏ��s", L"HTTP�ڑ��Ɏ��s", 0);
			return -2;
		}

		// HTTP�ڑ����J��
		m_hRequest = WinHttpOpenRequest(m_hConnect,
			wstrVerb,
			wstrObjectName,
			NULL,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			dwOpenRequestFlag);
		if (NULL == m_hRequest)
		{
			MessageBoxW(NULL, L"HTTP�ڑ����J���Ɏ��s", L"HTTP�ڑ����J���Ɏ��s", 0);
			return -3;
		}


		//// �R�[���o�b�N�֐��̐ݒ�
		//if (WINHTTP_INVALID_STATUS_CALLBACK == WinHttpSetStatusCallback(m_hConnect,
		//	(WINHTTP_STATUS_CALLBACK)InternetCallback,
		//	WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
		//	NULL))
		//{
		//	MessageBoxW(NULL, L"�R�[���o�b�N�̐ݒ�Ɏ��s", L"�R�[���o�b�N�̐ݒ�Ɏ��s", 0);
		//	return -4;
		//}

		while (!bDone)
		{
			//  If a proxy authentication challenge was responded to, reset 
			//  those credentials before each SendRequest, because the proxy  
			//  may require re-authentication after responding to a 401 or to 
			//  a redirect. If you don't, you can get into a 407-401-407-401
			//  loop.
			if (dwProxyAuthScheme != 0)
				bResults = WinHttpSetCredentials(m_hRequest,
				WINHTTP_AUTH_TARGET_PROXY,
				dwProxyAuthScheme,
				m_uid,
				m_pass,
				NULL);
			// Send a request.
			bResults = WinHttpSendRequest(m_hRequest,
				WINHTTP_NO_ADDITIONAL_HEADERS,
				0,
				WINHTTP_NO_REQUEST_DATA,
				0,
				0,
				0);

			// End the request.
			if (bResults)
				bResults = WinHttpReceiveResponse(m_hRequest, NULL);

			// Resend the request in case of 
			// ERROR_WINHTTP_RESEND_REQUEST error.
			if (!bResults && GetLastError() == ERROR_WINHTTP_RESEND_REQUEST)
				continue;

			// Check the status code.
			if (bResults)
				bResults = WinHttpQueryHeaders(m_hRequest,
				WINHTTP_QUERY_STATUS_CODE |
				WINHTTP_QUERY_FLAG_NUMBER,
				NULL,
				&dwStatusCode,
				&dwSize,
				NULL);

			if (bResults)
			{
				switch (dwStatusCode)
				{
				case 200:
					// The resource was successfully retrieved.
					// You can use WinHttpReadData to read the contents 
					// of the server's response.
					printf("The resource was successfully retrieved.\n");
					bDone = TRUE;

					// Verify available data.
					dwSize = 0;
					if (!WinHttpQueryDataAvailable(m_hRequest, &dwSize))
						printf("Error %u in WinHttpQueryDataAvailable.\n",
						GetLastError());

					// Allocate space for the buffer.
					pszOutBuffer = new char[dwSize + 1];
					if (!pszOutBuffer)
					{
						printf("Out of memory\n");
						dwSize = 0;
					}
					else
					{
						// Read the Data.
						ZeroMemory(pszOutBuffer, dwSize + 1);

						if (!WinHttpReadData(m_hRequest, (LPVOID)pszOutBuffer,
							dwSize, &dwDownloaded))
							printf("Error %u in WinHttpReadData.\n", GetLastError());
						else
							printf("%s\n", pszOutBuffer);


						// Success Tip
						Ballon("200", pszOutBuffer, NIIF_INFO, NULL);

						// Free the memory allocated to the buffer.
						delete[] pszOutBuffer;
					}

					bResults = CALLHTTPREQUEST_OK_FININSHED;

					break;

				case 401:
					// The server requires authentication.
					printf(
						"The server requires authentication. Sending credentials\n");

					// Obtain the supported and preferred schemes.
					bResults = WinHttpQueryAuthSchemes(m_hRequest,
						&dwSupportedSchemes,
						&dwFirstScheme,
						&dwTarget);

					// Set the credentials before re-sending the request.
					if (bResults)
					{
						dwSelectedScheme = ChooseAuthScheme(dwSupportedSchemes);

						if (dwSelectedScheme == 0)
							bDone = TRUE;
						else
							bResults = WinHttpSetCredentials(
							m_hRequest, dwTarget,
							dwSelectedScheme,
							m_uid,
							m_pass,
							NULL);
					}

					// If the same credentials are requested twice, abort the
					// request.  For simplicity, this sample does not check for
					// a repeated sequence of status codes.
					if (dwLastStatus == 401)
						bDone = TRUE;

					break;

				case 407:
					// The proxy requires authentication.
					printf(
						"The proxy requires authentication. Sending credentials\n");

					// Obtain the supported and preferred schemes.
					bResults = WinHttpQueryAuthSchemes(m_hRequest,
						&dwSupportedSchemes,
						&dwFirstScheme,
						&dwTarget);

					// Set the credentials before re-sending the request.
					if (bResults)
						dwProxyAuthScheme = ChooseAuthScheme(dwSupportedSchemes);

					// If the same credentials are requested twice, abort the
					// request.  For simplicity, this sample does not check for
					// a repeated sequence of status codes.
					if (dwLastStatus == 407)
						bDone = TRUE;
					break;

				default:
					// The status code does not indicate success.
					printf("Error. Status code %d returned.\n", dwStatusCode);
					bDone = TRUE;
				}
			}

			// Keep track of the last status code.
			dwLastStatus = dwStatusCode;

			// If there are any errors, break out of the loop.
			if (!bResults)
			{
				bDone = TRUE;
				// TipError

			}

		}

		// Report any errors.
		if (!bResults)
		{
			DWORD dwLastError = GetLastError();
			printf("Error %d has occurred.\n", dwLastError);
		}

		// Close any open handles.
		if (m_hRequest) WinHttpCloseHandle(m_hRequest);
		if (m_hConnect) WinHttpCloseHandle(m_hConnect);
		if (s_hSession) WinHttpCloseHandle(s_hSession);
	}

	return bResults;
}

BOOL Att() {

	DWORD bResult;

	//size_t origsize = strlen(m_urlKey) + 1;
	//const size_t newsize = 100;
	//size_t convertedChars = 0;
	//wchar_t wcstring[newsize];
	//ZeroMemory(wcstring, sizeof(wcstring));
	//wcscat_s(wcstring, L"xx");
	//mbstowcs_s(&convertedChars, &wcstring[lstrlenW(L"xx")], origsize, m_urlKey, _TRUNCATE);

	//bResult = CallHttpRequest(wcstring);
	bResult = CallHttpRequest(m_attAddr);
	if (bResult != CALLHTTPREQUEST_OK_FININSHED) {
		Ballon("ERROR", "ERROR", NIIF_ERROR, NULL);
	}

	return bResult;
}

BOOL Leave() {

	DWORD bResult;

	//size_t origsize = strlen(m_urlKey) + 1;
	//const size_t newsize = 100;
	//size_t convertedChars = 0;
	//wchar_t wcstring[newsize];
	//ZeroMemory(wcstring, sizeof(wcstring));
	//wcscat_s(wcstring, L"http://*/*/");
	//mbstowcs_s(&convertedChars, &wcstring[lstrlenW(L"http://*/*/")], origsize, m_urlKey, _TRUNCATE);

	//bResult = CallHttpRequest(wcstring);
	bResult = CallHttpRequest(m_leaveAddr);
	if (bResult != CALLHTTPREQUEST_OK_FININSHED) {
		Ballon("ERROR", "ERROR", NIIF_ERROR, NULL);
	}

	return bResult;
}

//
//   �֐�: InitInstance(HINSTANCE, int)
//
//   �ړI: �C���X�^���X �n���h����ۑ����āA���C�� �E�B���h�E���쐬���܂��B
//
//   �R�����g:
//
//        ���̊֐��ŁA�O���[�o���ϐ��ŃC���X�^���X �n���h����ۑ����A
//        ���C�� �v���O���� �E�B���h�E���쐬����ѕ\�����܂��B
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // �O���[�o���ϐ��ɃC���X�^���X�������i�[���܂��B
	//system("att.bat");

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	m_mainWnd = hWnd;

	// �C��
	Att();

	//ShowWindow(hWnd, nCmdShow);
	//UpdateWindow(hWnd);

	NOTIFYICONDATA notifyData;
	notifyData.cbSize = sizeof(notifyData);
	notifyData.hWnd = hWnd;
	notifyData.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	notifyData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	notifyData.uCallbackMessage = OA_MS_SYSTEMTRAY;
	notifyData.uVersion = NOTIFYICON_VERSION_4;
	lstrcpy(notifyData.szTip, "A-z");

	Shell_NotifyIcon(NIM_ADD, &notifyData);

	return TRUE;
}

BOOL Ballon(LPSTR title, LPSTR msg, DWORD dwType, UINT timeout) {

	NOTIFYICONDATA notifyIconData;
	notifyIconData.cbSize = sizeof(notifyIconData);
	notifyIconData.hWnd = m_mainWnd;

	lstrcpyn(notifyIconData.szInfoTitle, title, sizeof(notifyIconData.szInfoTitle));
	lstrcpyn(notifyIconData.szInfo, msg, sizeof(notifyIconData.szInfo));

	//dwType = 0;
	//dwType = NIIF_INFO;
	//dwType = NIIF_WARNING;
	//dwType = NIIF_ERROR;
	//dwType = NIIF_USER; // Use the "hBalloonIcon" parameter.
	//| (bSound ? 0 : NIIF_NOSOUND)
	//	| (bLargeIcon ? NIIF_LARGE_ICON : 0)
	//	| (bRespectQuiteTime ? NIIF_RESPECT_QUIET_TIME : 0);

	notifyIconData.dwInfoFlags = dwType;

	notifyIconData.uTimeout = timeout;
	notifyIconData.hBalloonIcon = NULL;
	notifyIconData.uFlags = NIF_INFO | NIF_GUID;

	if (dwType == NIIF_ERROR)
	{
		return MessageBox(m_mainWnd, "�ݒ�t�@�C���A�l�b�g���[�N�ڑ��󋵂��m�F���Ă��������I", "�ڑ��G���[", MB_ICONERROR);
	}
	else
	{
		return Shell_NotifyIcon(NIM_MODIFY, &notifyIconData);
	}

}


//
//  �֐�: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  �ړI:    ���C�� �E�B���h�E�̃��b�Z�[�W���������܂��B
//
//  WM_COMMAND	- �A�v���P�[�V���� ���j���[�̏���
//  WM_PAINT	- ���C�� �E�B���h�E�̕`��
//  WM_DESTROY	- ���~���b�Z�[�W��\�����Ė߂�
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// �I�����ꂽ���j���[�̉��:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDM_ATT:
		{
						// �C��
						Att();
		}
			break;
		case IDM_LEAVE:
		{
						  // �A�E�g
						  Leave();
		}
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: �`��R�[�h�������ɒǉ����Ă�������...
		EndPaint(hWnd, &ps);
		break;
	case WM_QUERYENDSESSION:
	{
							   // �A�E�g
							   Leave();

							   exit(0);
	}
		break;

	case WM_POWERBROADCAST:

		if (wParam == PBT_APMRESUMEAUTOMATIC)
		{
			// �C��
			Att();
		}
		else if (wParam == PBT_APMSUSPEND)
		{
			// �A�E�g
			Leave();
		}

		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case OA_MS_SYSTEMTRAY:

		switch (lParam)
		{

		case WM_LBUTTONDBLCLK:

			TrayMenu(hWnd);
			break;

		case WM_RBUTTONDOWN:

			TrayMenu(hWnd);
			break;

		}

		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void TrayMenu(HWND hWnd)
{

	BOOL    bResult = FALSE;
	DWORD   SelectionMade;

	int nIndex = 0;

	long nRecordNumber = 0;
	long nAbsolutePosition = 0;


	// menuRtClick.EnableMenuItem(ID_ITEM0, TRUE);
	// menuRtClick.EnableMenuItem(ID_ITEM1, TRUE);
	// menuRtClick.EnableMenuItem(ID_ITEM2, TRUE);


	// call the helper function to setup this as a titled popup menu
	// AddMenuTitle(popup);


	POINT pp;
	RECT rect;

	HMENU hpopMenu = NULL;
	HMENU hmenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU1));
	if (hmenu)
	{
		hpopMenu = GetSubMenu(hmenu, 0);

		RemoveMenu(hmenu, 0, MF_BYPOSITION);
		DestroyMenu(hmenu);
	}

	GetCursorPos(&pp);
	SelectionMade = TrackPopupMenu(hpopMenu,
		TPM_LEFTALIGN |
		TPM_LEFTBUTTON |
		TPM_RIGHTBUTTON |
		TPM_NOANIMATION |
		TPM_RETURNCMD,
		pp.x, pp.y, NULL, hWnd, &rect);

	// The value of SelectionMade is the id of the command selected or 0 if no 
	// selection was made

	switch (SelectionMade)
	{
	case ID_IN:
	{
		// �C��
		Att();
		break;
	}
	case ID_OUT:
	{
		// �A�E�g
		Leave();
		break;
	}
	case ID_0_EXIT:
	{
		exit(0);
		break;
	}
	}

	DestroyMenu(LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU1)));

	// display the popup menu
	// popup->TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this);

	// *pResult = 0;
}

// �o�[�W�������{�b�N�X�̃��b�Z�[�W �n���h���[�ł��B
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void CALLBACK InternetCallback(HINTERNET hInternet,
	DWORD_PTR dwContext,
	DWORD dwInternetStatus,
	LPVOID lpvStatusInformation,
	DWORD dwStatusInformationLength)
{

	printf_s("FK Complier");

	switch (dwInternetStatus)
	{
	case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
		// ���N�G�X�g�̑��M����
		OutputDebugString(_T("InternetStatus = SENDREQUEST_COMPLETE\n"));
		{
			//// ���X�|���X�̓����̑ҋ@
			//if (!WinHttpReceiveResponse(m_hRequest, NULL))
			//{
			//	printf_s(_T("WinHttpReceiveResponse()�Ɏ��s"));
			//	return;
			//}
		}
		break;
	case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
		// �w�b�_�[�̎擾�\
		OutputDebugString(_T("InternetStatus = HEADERS_AVAILABLE\n"));
		{
			//DWORD dwStatusCode = 0;
			//DWORD dwStatusCodeSize = sizeof(DWORD);
			//if (!WinHttpQueryHeaders(m_hRequest,
			//	WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,	// �X�e�[�^�X�R�[�h��DWORD�^�Ŏ擾����B
			//	WINHTTP_HEADER_NAME_BY_INDEX,	// �w�b�_�[���̃|�C���^
			//	&dwStatusCode,				// �o�b�t�@�[
			//	&dwStatusCodeSize,			// �o�b�t�@�[�T�C�Y
			//	WINHTTP_NO_HEADER_INDEX))		// �ŏ��ɔ��������w�b�_�[�̂ݎ��o��
			//{
			//	printf_s(_T("WinHttpQueryHeaders()�Ɏ��s"));
			//	return;
			//}
			//if (HTTP_STATUS_OK != dwStatusCode)
			//{
			//	printf_s(_T("�X�e�[�^�X�R�[�h�Ƃ���OK���Ԃ��Ă��Ȃ�����"));
			//	return;
			//}

			//// ���X�|���X�f�[�^�f�[�^�₢���킹
			//if (!WinHttpQueryDataAvailable(m_hRequest, NULL))
			//{
			//	printf_s(_T("WinHttpQueryDataAvailable()�Ɏ��s"));
			//	return;
			//}
		}
		break;
	case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
		// �f�[�^�̎擾�\
		OutputDebugString(_T("InternetStatus = DATA_AVAILABLE\n"));
		{
			//DWORD dwSize = *((LPDWORD)lpvStatusInformation);
			//if (0 == dwSize)
			//{	// �ǂݍ��ݏI��->�R�[���o�b�N�I��
			//	RequestSucceeded();
			//	return;
			//}

			//// ���X�|���X�f�[�^�ǂݍ���
			//DWORD dwLength = dwSize + 1;
			//char* pszBuffer = (char*)malloc(dwLength * sizeof(char));
			//if (!WinHttpReadData(m_hRequest,
			//	pszBuffer,		// �o�b�t�@�[
			//	dwSize,			// �ǂݍ��ރo�C�g��
			//	NULL))
			//{
			//	free(pszBuffer);
			//	printf_s(_T("WinHttpReadData()�Ɏ��s"));
			//	return;
			//}
			//// �o�b�t�@�[�͉���͂����ɏI���B
			//// WinHttpReadData()�����O�ɁA�R�[���o�b�N�֐���WINHTTP_CALLBACK_STATUS_READ_COMPLETE�ŌĂ΂��B
			//// �o�b�t�@�[�́A�R�[���o�b�N�֐���WINHTTP_CALLBACK_STATUS_READ_COMPLETE���ɏ�������B
		}
		break;
	case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
		OutputDebugString(_T("InternetStatus = READ_COMPLETE\n"));
		//if (lpvStatusInformation
		//	&& dwStatusInformationLength)
		//{
		//	char* pszBuffer = (char*)lpvStatusInformation;
		//	pszBuffer[dwStatusInformationLength] = '\0';
		//	m_ssRead << pszBuffer;
		//	free(pszBuffer);

		//	// ���X�|���X�f�[�^�f�[�^�₢���킹
		//	if (!WinHttpQueryDataAvailable(m_hRequest, NULL))
		//	{
		//		printf_s(_T("WinHttpQueryDataAvailable()�Ɏ��s"));
		//		return;
		//	}
		//}
		break;
	case WINHTTP_CALLBACK_STATUS_REDIRECT:
		OutputDebugString(_T("InternetStatus = REDIRECT\n"));
		break;
	case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
		OutputDebugString(_T("InternetStatus = REQUEST_ERROR\n"));
		printf_s(_T("InternetStatus = REQUEST_ERROR"));
		return;
	case WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE:
		OutputDebugString(_T("InternetStatus = INTERMEDIATE_RESPONSE\n"));
		break;
	default:
		OutputDebugString(_T("Unknown InternetStatus \n"));
		printf_s(_T("Unknown InternetStatus"));
		return;
	}
}