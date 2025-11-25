#pragma once

#ifndef  _WINDOWS_H
#include <Windows.h>
#define _WINDOWS_H
#endif // ! _WINDOWS_H

#include <TlHelp32.h>

struct SModule
{
	DWORD dwBase;
	DWORD dwSize;
};

template<typename T, size_t N>
size_t countof(T(&array)[N])
{
	return N;
}

class Debugger
{
public:
	HANDLE Process;
private:
	DWORD ProcessID;
public:
	bool Attach(char* process, DWORD rights = PROCESS_ALL_ACCESS)
	{
		HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(entry);
		do {
			if (!strcmp(entry.szExeFile, process)) {
				ProcessID = entry.th32ProcessID;
				CloseHandle(handle);
				Process = OpenProcess(rights, false, ProcessID);
				return true;
			}
		} while (Process32Next(handle, &entry));
		return false;
	}
	SModule GetModule(char* modulename)
	{
		HANDLE module = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessID);
		MODULEENTRY32 mEntry;
		mEntry.dwSize = sizeof(mEntry);
		do {
			if (!strcmp(mEntry.szModule, modulename))
			{
				SModule mod = { (DWORD)mEntry.hModule, mEntry.modBaseSize };
				return mod;
			}
		} while (Module32Next(module, &mEntry));
		return { (DWORD)false, (DWORD)false };
	}

	template<typename T>
	T Read(DWORD address)
	{
		T _read;
		ReadProcessMemory(Process, (LPVOID)address, &_read, sizeof(T), NULL);
		return _read;
	}
	template<typename T>
	void Write(DWORD address, T value)
	{
		WriteProcessMemory(Process, (LPVOID)address, &value, sizeof(T), NULL);
	}

	bool DataCompare(const BYTE* data, const BYTE* mask, LPCSTR szMask)
	{
		for (; *szMask; ++szMask, ++data, ++mask)
			if (*szMask == 'x' && *data != *mask)
				return false;
		return (*szMask == NULL);
	}

	DWORD FindPattern(DWORD start, DWORD size, LPCSTR sig, LPCSTR mask)
	{
		BYTE* data = new BYTE[size];
		SIZE_T bytesread;
		if (!ReadProcessMemory(Process, (LPVOID)start, data, size, &bytesread))
		{
			return NULL;
		}
		for (DWORD i = 0; i < size; i++)
		{
			if (DataCompare((CONST BYTE*)(data + i), (CONST BYTE*)sig, mask))
			{
				return start + i;
			}
		}
		return NULL;
	}
	DWORD FindPatternArray(DWORD start, DWORD size, LPCSTR mask, int count, ...)
	{
		char* sig = new char[count + 1];
		va_list ap;
		va_start(ap, count);
		for (int i = 0; i < count; i++)
		{
			char read = va_arg(ap, char);
			sig[i] = read;
		}
		va_end(ap);
		sig[count] = '\0';
		return FindPattern(start, size, sig, mask);
	}
};