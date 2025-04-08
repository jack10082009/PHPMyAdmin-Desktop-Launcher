// +----------------------------------------------------------------------
// | PHPMyAdmin桌面启动器 | PHPMyAdmin Desktop Launcher for Windows
// | 一个封装好了的PHPMyAdmin桌面启动器，使用PHP内置服务器和浏览器打开
// | 支持Chrome、Edge、Firefox浏览器，支持应用模式和隐私模式
// | A PHPMyAdmin desktop launcher that uses the built-in PHP server and browser to open it
// | Supports Chrome, Edge, and Firefox browsers, as well as app mode and incognito mode
// +----------------------------------------------------------------------
// | @Environment: PHP 7.4.33 + PHPMyAdmin 5.2.2 on Windows Platform
// | @Author:  Jack Wang <jack10082009@GitHub>
// | @Date:    2025-04-09 00:39:00 +0800
// | @Version: 1.0.0
// | @License: MIT
// +----------------------------------------------------------------------
#include <windows.h>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <tlhelp32.h>
#pragma execution_character_set("utf-8")
static HINSTANCE hInst;
static HWND hStatusText;
static PROCESS_INFORMATION piServer;
int PORT = 7000;
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
			switch (wmId) {
				case 1: OpenBrowser(L"chrome.exe", true); break;
				case 2: OpenBrowser(L"chrome.exe", false); break;
				case 3: OpenBrowser(L"msedge.exe", true); break;
				case 4: OpenBrowser(L"msedge.exe", false); break;
				case 5: OpenBrowser(L"chrome.exe", false, true); break;
				case 6: OpenBrowser(L"msedge.exe", false, true); break;
				case 7: OpenBrowser(L"firefox.exe", false); break;
				case 8: OpenBrowser(L"firefox.exe", false, true); break;
				case 99: DestroyWindow(hWnd); break;
			}
			break;
		}
		case WM_DESTROY: {
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
	wcex.lpszClassName = L"PHPMyAdminLauncher";
	RegisterClassExW(&wcex);
	HWND hWnd = CreateWindowW(
		L"PHPMyAdminLauncher",
		L"PHPMyAdmin Desktop Launcher/PHPMyAdmin桌面启动器",
		WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
		CW_USEDEFAULT, 0, 360, 160,
		nullptr, nullptr, hInstance, nullptr
		);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	if(hStatusText){
		std::wstring msg = (piServer.dwProcessId != 0) ?
		L"http://localhost:" + std::to_wstring(PORT) :
		L"启动失败/Launch failed";
		SetWindowTextW(hStatusText, msg.c_str());
	}
	MSG msg;
	while(GetMessage(&msg, nullptr, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}