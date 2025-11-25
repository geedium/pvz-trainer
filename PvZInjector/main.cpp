#include <iostream>
#include <cwchar>
#include <stdlib.h>
#include "Utils.h"
#include <windows.h>
#include <tlhelp32.h>
#include <string>

DWORD getPidByName(const std::wstring& processName) {
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) return 0;

	if (Process32First(hProcessSnap, &pe32)) {
		do {
			if (_wcsicmp(processName.c_str(), pe32.szExeFile) == 0) {
				CloseHandle(hProcessSnap);
				return pe32.th32ProcessID;
			}
		} while (Process32Next(hProcessSnap, &pe32));
	}

	CloseHandle(hProcessSnap);
	return 0;
}

bool InjectDLL(DWORD pid, const char* dllPath) {
	char fullPath[MAX_PATH];
	GetFullPathNameA(dllPath, MAX_PATH, fullPath, nullptr);

	HANDLE hProcess = OpenProcess(
		PROCESS_CREATE_THREAD |
		PROCESS_QUERY_INFORMATION |
		PROCESS_VM_OPERATION |
		PROCESS_VM_WRITE |
		PROCESS_VM_READ,
		FALSE,
		pid
	);

	if (!hProcess) return false;

	LPVOID alloc = VirtualAllocEx(
		hProcess,
		nullptr,
		strlen(fullPath) + 1,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE
	);

	if (!alloc) return false;

	WriteProcessMemory(hProcess, alloc, fullPath, strlen(fullPath) + 1, nullptr);

	LPTHREAD_START_ROUTINE loadLibraryA =
		(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

	HANDLE hThread = CreateRemoteThread(
		hProcess, nullptr, 0, loadLibraryA, alloc, 0, nullptr
	);

	WaitForSingleObject(hThread, INFINITE);

	VirtualFreeEx(hProcess, alloc, 0, MEM_RELEASE);
	CloseHandle(hThread);
	CloseHandle(hProcess);

	return true;
}

int main() {
	const char* dll = "PvZHook.dll";
	std::wstring process = L"popcapgame1.exe";
	DWORD pid = getPidByName(process);

	if (InjectDLL(pid, dll))
		MessageBoxA(0, "Injected successfully!", "OK", 0);
	else
		MessageBoxA(0, "Injection failed!", "Error", MB_ICONERROR);

	return 0;
}