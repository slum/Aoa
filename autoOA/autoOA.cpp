// autoOA.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "autoOA.h"

#define MAX_LOADSTRING 100
#define INQ_TIMER_ID 1000

// グローバル変数:
HINSTANCE hInst;								// 現在のインターフェイス
HWND m_mainWnd;
TCHAR szTitle[MAX_LOADSTRING];					// タイトル バーのテキスト
TCHAR szWindowClass[MAX_LOADSTRING];			// メイン ウィンドウ クラス名

LPTSTR m_urlKey; // 社員ナンバー/キー 
LPTSTR m_inqSec;
LPTSTR m_lastHour;
LPCWCHAR m_uid;
LPCWCHAR m_pass;



// このコード モジュールに含まれる関数の宣言を転送します:
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
	HWND hwnd,         // ウィンドウのハンドル
	UINT uMsg,         // WM_TIMER メッセージ
	UINT_PTR idEvent,  // タイマの識別子
	DWORD dwTime       // 現在のシステム時刻
);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: ここにコードを挿入してください。
	MSG msg;
	HACCEL hAccelTable;

	// グローバル文字列を初期化しています。
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_AUTOOA, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	WCHAR uid[100];
	WCHAR upassword[100];
	CHAR urlKey[100];
	CHAR inqSec[100];
	CHAR lastHour[100];
	m_uid = uid;
	m_pass = upassword;
	m_urlKey = urlKey;
	m_inqSec = inqSec;
	m_lastHour = lastHour;

	if (lstrlen(lpCmdLine) == 0) {

		GetPrivateProfileStringW(L"UserData", L"Uid", L"", uid, sizeof(uid), L"ini\\autoOA.ini");
		GetPrivateProfileStringW(L"UserData", L"UPassword", L"", upassword, sizeof(upassword), L"ini\\autoOA.ini");
		GetPrivateProfileString("InqTime", "InqTime", "", inqSec, sizeof(inqSec), "ini\\autoOA.ini");
		GetPrivateProfileString("lastHour", "lastHour", "", lastHour, sizeof(lastHour), "ini\\autoOA.ini");
		GetPrivateProfileString("UserData", "Key", "", urlKey, sizeof(urlKey), "ini\\autoOA.ini");
	}
	else {

		WCHAR wcstring[100];
		ZeroMemory(wcstring, sizeof(wcstring));
		size_t convertedChars = 0;
		size_t orgChars = 0;

		mbstowcs_s(&convertedChars, &wcstring[0], lstrlen(lpCmdLine) + 1, lpCmdLine, _TRUNCATE);

		GetPrivateProfileStringW(L"UserData", L"Uid", L"", uid, sizeof(uid), wcstring);
		GetPrivateProfileStringW(L"UserData", L"UPassword", L"", upassword, sizeof(upassword), wcstring);
		GetPrivateProfileString("InqTime", "InqTime", "", inqSec, sizeof(inqSec), lpCmdLine);
		GetPrivateProfileString("lastHour", "lastHour", "", lastHour, sizeof(lastHour), lpCmdLine);
		GetPrivateProfileString("UserData", "Key", "", urlKey, sizeof(urlKey), lpCmdLine);

	}

	// アプリケーションの初期化を実行します:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_AUTOOA));

	SetTimer(m_mainWnd, INQ_TIMER_ID, atoi(m_inqSec), TimerProc);

	// メイン メッセージ ループ:
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
	HWND hwnd,         // ウィンドウのハンドル
	UINT uMsg,         // WM_TIMER メッセージ
	UINT_PTR idEvent,  // タイマの識別子
	DWORD dwTime       // 現在のシステム時刻
) {

	SYSTEMTIME time;

	GetLocalTime(&time);

	if (atoi(m_lastHour) >= time.wHour) {
		Leave();
	}

	KillTimer(m_mainWnd, INQ_TIMER_ID);
}


//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_AUTOOA));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_AUTOOA);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します。
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
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

			MessageBoxW(NULL, L"URL解析に失敗", L"URL解析に失敗", 0);
			return -1;
		}

		wstrServer = urlcomponents.lpszHostName;
		wstrObjectName = urlcomponents.lpszUrlPath;
		nPort = urlcomponents.nPort;

		// HTTPかHTTPSかそれ以外か
		DWORD dwOpenRequestFlag = (INTERNET_SCHEME_HTTPS == urlcomponents.nScheme) ? WINHTTP_FLAG_SECURE : 0;

		// POSTかGETか
		if (false)
		{	// POST
			wstrVerb = L"POST";
			wstrHeaders = L"Content-Type: application/x-www-form-urlencoded";
			//if (0 != tstrParameter.length())
			//{	// パラメータを、送信するオプションデータに変換する
			//	char* pszOptional = NhT2M(tstrParameter.c_str());	// char文字列に変換
			//	strOptional = pszOptional;
			//	free(pszOptional);
			//}
		}
		else
		{	// GET
			wstrVerb = L"GET";
			wstrHeaders = L"";
			//if (0 != tstrParameter.length())
			//{	// オブジェクトとパラメータを「?」で連結
			//	WCHAR* pwszBuffer = NhT2W(tstrParameter.c_str());
			//	wstrObjectName += L"?" + wstring(pwszBuffer);
			//	free(pwszBuffer);
			//}
		}

		// HTTP接続
		m_hConnect = WinHttpConnect(s_hSession,
			wstrServer,
			nPort,
			0);
		if (NULL == m_hConnect)
		{
			MessageBoxW(NULL, L"HTTP接続に失敗", L"HTTP接続に失敗", 0);
			return -2;
		}

		// HTTP接続を開く
		m_hRequest = WinHttpOpenRequest(m_hConnect,
			wstrVerb,
			wstrObjectName,
			NULL,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			dwOpenRequestFlag);
		if (NULL == m_hRequest)
		{
			MessageBoxW(NULL, L"HTTP接続を開くに失敗", L"HTTP接続を開くに失敗", 0);
			return -3;
		}


		//// コールバック関数の設定
		//if (WINHTTP_INVALID_STATUS_CALLBACK == WinHttpSetStatusCallback(m_hConnect,
		//	(WINHTTP_STATUS_CALLBACK)InternetCallback,
		//	WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
		//	NULL))
		//{
		//	MessageBoxW(NULL, L"コールバックの設定に失敗", L"コールバックの設定に失敗", 0);
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

	size_t origsize = strlen(m_urlKey) + 1;
	const size_t newsize = 100;
	size_t convertedChars = 0;
	wchar_t wcstring[newsize];
	ZeroMemory(wcstring, sizeof(wcstring));
	wcscat_s(wcstring, L"http://*/*/");
	mbstowcs_s(&convertedChars, &wcstring[lstrlenW(L"http://*/*/")], origsize, m_urlKey, _TRUNCATE);

	bResult = CallHttpRequest(wcstring);
	if (bResult != CALLHTTPREQUEST_OK_FININSHED) {
		Ballon("ERROR", "ERROR", NIIF_ERROR, NULL);
	}

	return bResult;
}

BOOL Leave() {

	DWORD bResult;

	size_t origsize = strlen(m_urlKey) + 1;
	const size_t newsize = 100;
	size_t convertedChars = 0;
	wchar_t wcstring[newsize];
	ZeroMemory(wcstring, sizeof(wcstring));
	wcscat_s(wcstring, L"http://*/*/");
	mbstowcs_s(&convertedChars, &wcstring[lstrlenW(L"http://*/*/")], origsize, m_urlKey, _TRUNCATE);

	bResult = CallHttpRequest(wcstring);
	if (bResult != CALLHTTPREQUEST_OK_FININSHED) {
		Ballon("ERROR", "ERROR", NIIF_ERROR, NULL);
	}

	return bResult;
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します。
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // グローバル変数にインスタンス処理を格納します。
   //system("att.bat");

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   m_mainWnd = hWnd;

   // イン
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
		return MessageBox(m_mainWnd, "設定ファイル、ネットワーク接続状況を確認してください！", "接続エラー", MB_ICONERROR);
	} else 
	{
		return Shell_NotifyIcon(NIM_MODIFY, &notifyIconData);
	}

}


//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND	- アプリケーション メニューの処理
//  WM_PAINT	- メイン ウィンドウの描画
//  WM_DESTROY	- 中止メッセージを表示して戻る
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
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 選択されたメニューの解析:
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
			// イン
			Att();
		}
			break;
		case IDM_LEAVE:
		{
			// アウト
			Leave();
		}
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: 描画コードをここに追加してください...
		EndPaint(hWnd, &ps);
		break;
	case WM_QUERYENDSESSION:
	{
		// アウト
		Leave();

		exit(0);
	}
		break;

	case WM_POWERBROADCAST:

		if (wParam == PBT_APMRESUMEAUTOMATIC)
		{
			// イン
			Att();
		}
		else if (wParam == PBT_APMSUSPEND) 
		{
			// アウト
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
		// イン
		Att();
		break;
	}
	case ID_OUT:
	{
		// アウト
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

// バージョン情報ボックスのメッセージ ハンドラーです。
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
		// リクエストの送信完了
		OutputDebugString(_T("InternetStatus = SENDREQUEST_COMPLETE\n"));
		{
			//// レスポンスの到着の待機
			//if (!WinHttpReceiveResponse(m_hRequest, NULL))
			//{
			//	printf_s(_T("WinHttpReceiveResponse()に失敗"));
			//	return;
			//}
		}
		break;
	case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
		// ヘッダーの取得可能
		OutputDebugString(_T("InternetStatus = HEADERS_AVAILABLE\n"));
		{
			//DWORD dwStatusCode = 0;
			//DWORD dwStatusCodeSize = sizeof(DWORD);
			//if (!WinHttpQueryHeaders(m_hRequest,
			//	WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,	// ステータスコードをDWORD型で取得する。
			//	WINHTTP_HEADER_NAME_BY_INDEX,	// ヘッダー名のポインタ
			//	&dwStatusCode,				// バッファー
			//	&dwStatusCodeSize,			// バッファーサイズ
			//	WINHTTP_NO_HEADER_INDEX))		// 最初に発生したヘッダーのみ取り出す
			//{
			//	printf_s(_T("WinHttpQueryHeaders()に失敗"));
			//	return;
			//}
			//if (HTTP_STATUS_OK != dwStatusCode)
			//{
			//	printf_s(_T("ステータスコードとしてOKが返ってこなかった"));
			//	return;
			//}
	
			//// レスポンスデータデータ問い合わせ
			//if (!WinHttpQueryDataAvailable(m_hRequest, NULL))
			//{
			//	printf_s(_T("WinHttpQueryDataAvailable()に失敗"));
			//	return;
			//}
		}
		break;
	case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
		// データの取得可能
		OutputDebugString(_T("InternetStatus = DATA_AVAILABLE\n"));
		{
			//DWORD dwSize = *((LPDWORD)lpvStatusInformation);
			//if (0 == dwSize)
			//{	// 読み込み終了->コールバック終了
			//	RequestSucceeded();
			//	return;
			//}
	
			//// レスポンスデータ読み込み
			//DWORD dwLength = dwSize + 1;
			//char* pszBuffer = (char*)malloc(dwLength * sizeof(char));
			//if (!WinHttpReadData(m_hRequest,
			//	pszBuffer,		// バッファー
			//	dwSize,			// 読み込むバイト数
			//	NULL))
			//{
			//	free(pszBuffer);
			//	printf_s(_T("WinHttpReadData()に失敗"));
			//	return;
			//}
			//// バッファーは解放はせずに終了。
			//// WinHttpReadData()完了前に、コールバック関数がWINHTTP_CALLBACK_STATUS_READ_COMPLETEで呼ばれる。
			//// バッファーは、コールバック関数のWINHTTP_CALLBACK_STATUS_READ_COMPLETE時に処理する。
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
	
		//	// レスポンスデータデータ問い合わせ
		//	if (!WinHttpQueryDataAvailable(m_hRequest, NULL))
		//	{
		//		printf_s(_T("WinHttpQueryDataAvailable()に失敗"));
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
