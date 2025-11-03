// +----------------------------------------------------------------------
// | PHPMyAdmin桌面启动器 | PHPMyAdmin Desktop Launcher for Windows
// | 一个封装好了的PHPMyAdmin桌面启动器，使用PHP内置服务器和浏览器打开
// | 支持Chrome、Edge、Firefox浏览器，支持应用模式和隐私模式
// | A PHPMyAdmin desktop launcher that uses the built-in PHP server and browser to open it
// | Supports Chrome, Edge, and Firefox browsers, as well as app mode and incognito mode
// +----------------------------------------------------------------------
// | @Environment: PHP 7.4.33 + PHPMyAdmin 5.2.2 on Windows Platform
// | @Author:  Jack Wang <jack10082009@GitHub>
// | @Date:    2025-11-03 16:30:00 +0800
// | @Version: 1.1.0
// | @License: MIT
// +----------------------------------------------------------------------
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <tlhelp32.h>
#pragma execution_character_set("utf-8")
#define IDI_ICON1 101
static HINSTANCE hInst;
static HWND hStatusText;
static HWND hMainWnd;
static PROCESS_INFORMATION piServer;
static NOTIFYICONDATAW nid;
int PORT = 7000;
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_RESTORE 1001
#define ID_TRAY_OPEN_REPO 1003
#define ID_TRAY_EXIT 1002
/**
 * 检索 PHP 可执行文件路径
 * Retrieves the PHP executable path
 * @return The path as a wide string.
 */
std::wstring GetExePath(){
	wchar_t path[MAX_PATH];
	GetModuleFileNameW(NULL, path, MAX_PATH);
	std::wstring wsPath(path);
	return wsPath.substr(0, wsPath.find_last_of(L"\\/"));
}
/**
 * 更新 PHP 配置文件,因为对于PHP可执行文件来说，需要用绝对路径找到扩展目录,否则无法加载MySQLi等扩展
 * Updates the PHP configuration file, because for the PHP executable,
 * it needs to find the extension directory with an absolute path,
 * otherwise it cannot load extensions like MySQLi.
 * @return true if successful, false otherwise.
 */
bool UpdatePHPConfig(){
	std::wstring exePath = GetExePath();
	std::wstring phpIniPath = exePath + L"\\php\\php.ini";
	std::wstring extPath = exePath + L"\\php\\ext";
	std::wifstream in(phpIniPath.c_str());
	if(!in) return false;
	std::wstringstream buffer;
	std::wstring line;
	bool found = false;
	// 读取配置文件并查找 extension_dir 行
	// Read the configuration file and look for the extension_dir line
	while(getline(in, line)){
		if(line.find(L"extension_dir") != std::wstring::npos && line[0] != L';'){
			buffer << L"extension_dir = \"" << extPath << L"\"" << std::endl;
			found = true;
		}else{
			buffer << line << std::endl;
		}
	}
	in.close();
	if(!found){
		buffer << L"\nextension_dir = \"" << extPath << L"\"" << std::endl;
	}
	std::wofstream out(phpIniPath.c_str());
	out << buffer.str();
	out.close();
	return true;
}
/**
 * 启动 PHP 内置服务器
 * Starts the PHP built-in server
 * @param port The port to use.
 * @return true if successful, false otherwise.
 */
bool StartPHPServer(int& port) {
	std::wstring exePath = GetExePath();
	std::wstring phpExe = exePath + L"\\php\\php.exe";
	std::wstring webRoot = exePath + L"\\web";
	for(; port <= 7200; port += 9){
		std::wstring cmd = L"\"" + phpExe + L"\" -S localhost:" + std::to_wstring(port) + L" -t \"" + webRoot + L"\"";
		STARTUPINFOW si = { sizeof(si) };
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		wchar_t cmdLine[1024];
		wcscpy_s(cmdLine, cmd.c_str());
		if(CreateProcessW(
			phpExe.c_str(),
			cmdLine,
			NULL,
			NULL,
			FALSE,
			CREATE_NO_WINDOW,
			NULL,
			NULL,
			&si,
			&piServer)){return true;}
	}
	return false;
}
/**
 * 添加托盘图标
 * Adds a tray icon
 * @param hWnd The window handle.
 * @return true if successful, false otherwise.
 */
bool AddTrayIcon(HWND hWnd) {
	ZeroMemory(&nid, sizeof(NOTIFYICONDATAW));
	nid.cbSize = sizeof(NOTIFYICONDATAW);
	nid.hWnd = hWnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_TRAYICON;
	nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
	wcscpy_s(nid.szTip, L"PHPMyAdmin Desktop Launcher");
	return Shell_NotifyIconW(NIM_ADD, &nid);
}
/**
 * 移除托盘图标
 * Removes the tray icon
 */
void RemoveTrayIcon() {
	Shell_NotifyIconW(NIM_DELETE, &nid);
}
/**
 * 显示托盘菜单
 * Shows the tray menu
 * @param hWnd The window handle.
 */
void ShowTrayMenu(HWND hWnd) {
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();
	AppendMenuW(hMenu, MF_STRING, ID_TRAY_RESTORE, L"恢复窗口/Restore");
	AppendMenuW(hMenu, MF_STRING, ID_TRAY_OPEN_REPO, L"打开仓库/Open Repository");
	AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"退出/Exit");
	SetForegroundWindow(hWnd);
	TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
	PostMessage(hWnd, WM_NULL, 0, 0);
	DestroyMenu(hMenu);
}
/**
 * 递归终止进程树
 * Recursively terminates the process tree
 * @param pid The process ID to terminate.
 */
void KillProcessTree(DWORD pid) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(hSnapshot == INVALID_HANDLE_VALUE) return;
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);
	if(Process32First(hSnapshot, &pe)){
		do {
			if(pe.th32ParentProcessID == pid){
				KillProcessTree(pe.th32ProcessID);
			}
		}while(Process32Next(hSnapshot, &pe));
	}
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
	if(hProcess){
		TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
	}
	CloseHandle(hSnapshot);
}
/**
 * 打开浏览器
 * Opens the browser
 * @param browser The browser executable name.
 * @param appMode Whether to open in app mode.
 * @param incognito Whether to open in incognito mode.
 */
void OpenBrowser(const std::wstring& browser, bool appMode, bool incognito = false) {
	std::wstring url = L"http://localhost:" + std::to_wstring(PORT);
	std::wstring args;
	if(browser.find(L"chrome") != std::wstring::npos){
		if(appMode){args = L"--app=" + url;
		}else{args = url;}
		if(incognito){args = L"--incognito " + args;}
	}else if(browser.find(L"msedge") != std::wstring::npos){
		if(appMode){args = L"--app=" + url;
		}else{args = url;}
		if(incognito){args = L"--inprivate " + args;}
	}else if(browser.find(L"firefox") != std::wstring::npos){
		args = url;
		if(incognito){args = L"-private-window " + args;}
	}
	ShellExecuteW(NULL, L"open", browser.c_str(), args.c_str(), NULL, SW_SHOWNORMAL);
}
/**
 * 解析命令行参数中的option值
 * Parses the option value from command line arguments
 * @param cmdLine The command line string.
 * @return The option value (1-8), or 0 if invalid or not found.
 */
int ParseOptionFromCmdLine(LPSTR cmdLine) {
	if(!cmdLine || strlen(cmdLine) == 0) return 0;
	std::string cmd(cmdLine);
	size_t pos = cmd.find("-option=");
	if(pos == std::string::npos) return 0;
	pos += 8;
	if(pos >= cmd.length()) return 0;
	int option = 0;
	while(pos < cmd.length() && cmd[pos] >= '0' && cmd[pos] <= '9'){
		option = option * 10 + (cmd[pos] - '0');
		pos++;
	}
	if(option >= 1 && option <= 8){return option;}
	return 0;
}
/**
 * 检测命令行参数中是否包含-mini
 * Checks if the command line arguments contain -mini
 * @param cmdLine The command line string.
 * @return true if -mini is found, false otherwise.
 */
bool ParseMiniFromCmdLine(LPSTR cmdLine) {
	if(!cmdLine || strlen(cmdLine) == 0) return false;
	std::string cmd(cmdLine);
	return cmd.find("-mini") != std::string::npos;
}
/**
 * 根据option参数打开对应的浏览器
 * Opens the corresponding browser based on the option parameter
 * @param option The option value (1-8).
 * @return true if the option is valid and browser is opened, false otherwise.
 */
bool OpenBrowserByOption(int option) {
	switch(option){
		case 1: OpenBrowser(L"chrome.exe", true); return true;          // Chrome-APP
		case 2: OpenBrowser(L"chrome.exe", false); return true;         // Chrome
		case 3: OpenBrowser(L"chrome.exe", false, true); return true;   // Chrome-Incognito
		case 4: OpenBrowser(L"msedge.exe", true); return true;          // Edge-APP
		case 5: OpenBrowser(L"msedge.exe", false); return true;         // Edge
		case 6: OpenBrowser(L"msedge.exe", false, true); return true;   // Edge-InPrivate
		case 7: OpenBrowser(L"firefox.exe", false); return true;        // Firefox
		case 8: OpenBrowser(L"firefox.exe", false, true); return true;  // Firefox-Private
		default: return false;  // 无效选项/Invalid option
	}
}
/**
 * 窗口过程函数
 * Window procedure function
 * @param hWnd The window handle.
 * @param message The message identifier.
 * @param wParam The first message parameter.
 * @param lParam The second message parameter.
 * @return The result of the message processing.
 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_CREATE: {
			hStatusText = CreateWindowW(
				L"STATIC",
				L"Starting Server...",
				WS_VISIBLE | WS_CHILD | SS_LEFT,
				10, 10, 330, 25,
				hWnd,
				NULL,
				hInst,
				NULL
				);
			//Chrome-APP, Chrome, Chrome-Incognito
			CreateWindowW(L"BUTTON", L"Chrome-APP", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
				10, 40, 110, 28, hWnd, (HMENU)1, hInst, NULL);
			CreateWindowW(L"BUTTON", L"Chrome", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
				120, 40, 80, 28, hWnd, (HMENU)2, hInst, NULL);
			CreateWindowW(L"BUTTON", L"Chrome-Incognito", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
				200, 40, 140, 28, hWnd, (HMENU)5, hInst, NULL);
			//Edge-APP, Edge, Edge-InPrivate
			CreateWindowW(L"BUTTON", L"Edge-APP", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
				10, 68, 110, 28, hWnd, (HMENU)3, hInst, NULL);
			CreateWindowW(L"BUTTON", L"Edge", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
				120, 68, 80, 28, hWnd, (HMENU)4, hInst, NULL);
			CreateWindowW(L"BUTTON", L"Edge-InPrivate", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
				200, 68, 140, 28, hWnd, (HMENU)6, hInst, NULL);
			//Firefox, Firefox-Private, Exit(退出)
			CreateWindowW(L"BUTTON", L"Firefox", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
				10, 96, 110, 28, hWnd, (HMENU)7, hInst, NULL);
			CreateWindowW(L"BUTTON", L"Firefox-Private", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
				120, 96, 140, 28, hWnd, (HMENU)8, hInst, NULL);
			CreateWindowW(L"BUTTON", L"Exit", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
				260, 96, 80, 28, hWnd, (HMENU)99, hInst, NULL);
			
			break;
		}
		case WM_COMMAND: {
			int wmId = LOWORD(wParam);
			if(wmId == 99 || wmId == ID_TRAY_EXIT){
				DestroyWindow(hWnd);
			}else if(wmId == ID_TRAY_RESTORE){
				ShowWindow(hWnd, SW_RESTORE);
				SetForegroundWindow(hWnd);
			}else if(wmId == ID_TRAY_OPEN_REPO){
				ShellExecuteW(NULL, L"open", L"https://github.com/jack10082009/PHPMyAdmin-Desktop-Launcher", NULL, NULL, SW_SHOWNORMAL);
			}else{
				OpenBrowserByOption(wmId);
			}
			break;
		}
		case WM_SIZE: {
			if(wParam == SIZE_MINIMIZED){
				ShowWindow(hWnd, SW_HIDE);
			}
			break;
		}
		case WM_TRAYICON: {
			if(lParam == WM_LBUTTONDBLCLK){
				ShowWindow(hWnd, SW_RESTORE);
				SetForegroundWindow(hWnd);
			}else if(lParam == WM_RBUTTONUP){
				ShowTrayMenu(hWnd);
			}
			break;
		}
		case WM_DESTROY: {
			RemoveTrayIcon();
			KillProcessTree(piServer.dwProcessId);
			CloseHandle(piServer.hProcess);
			CloseHandle(piServer.hThread);
			PostQuitMessage(0);
			break;
		}
	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return 0;
}
/**
 * 主函数
 * Main function
 * @param hInstance The instance handle.
 * @param hPrevInstance The previous instance handle.
 * @param lpCmdLine The command line arguments.
 * @param nCmdShow The command show flag.
 * @return The exit code.
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	hInst = hInstance;
	int autoOption = ParseOptionFromCmdLine(lpCmdLine);
	bool autoMini = ParseMiniFromCmdLine(lpCmdLine);
	if(!UpdatePHPConfig()){
		MessageBoxW(NULL, L"无法更新PHP配置。/Unable to update PHP configuration", L"错误/Error", MB_ICONERROR);
		return 1;
	}
	if(!StartPHPServer(PORT)){
		MessageBoxW(NULL, L"PHP服务器启动失败，端口已用尽。/PHP server failed to start, port exhausted.", L"错误/Error", MB_ICONERROR);
	}
	WNDCLASSEXW wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wcex.lpszClassName = L"PHPMyAdminLauncher";
	RegisterClassExW(&wcex);
	HWND hWnd = CreateWindowW(
		L"PHPMyAdminLauncher",
		L"PHPMyAdmin Desktop Launcher/PHPMyAdmin桌面启动器",
		WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
		CW_USEDEFAULT, 0, 360, 160,
		nullptr, nullptr, hInstance, nullptr
		);
	hMainWnd = hWnd;
	AddTrayIcon(hWnd);
	// 如果有-mini参数，自动隐藏窗口
	// If -mini parameter is present, automatically hide the window
	if(autoMini){
		ShowWindow(hWnd, SW_HIDE);
	}else{
		ShowWindow(hWnd, nCmdShow);
	}
	UpdateWindow(hWnd);
	if(hStatusText){
		std::wstring msg = (piServer.dwProcessId != 0) ?
		L"http://localhost:" + std::to_wstring(PORT) :
		L"启动失败/Launch failed";
		SetWindowTextW(hStatusText, msg.c_str());
	}
	// 如果服务器启动成功并且有有效的option参数，自动打开浏览器
	// If the server started successfully and there's a valid option parameter, auto-open browser
	if(piServer.dwProcessId != 0 && autoOption >= 1 && autoOption <= 8){
		Sleep(500);
		OpenBrowserByOption(autoOption);
	}
	MSG msg;
	while(GetMessage(&msg, nullptr, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}