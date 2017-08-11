// Fritz Ammon
// Skype DLL Injector
// Theory
// Copyright © 2014
// LoadLibrary is a function in KERNEL32 which can be called by all
// applications. Probably because the function address loads into the
// same place for all applications once the DLL is loaded as it is a
// system DLL. We might be able to get the address of the Winsock recv
// function, create a detour, and then create a remote thread from
// the DLL back to here whenever a packet is received.

#define _CRT_SECURE_NO_DEPRECATE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#pragma comment(lib, "Winmm")

#include <iostream>
#include <exception>
#include <Windows.h>
#include <MMSystem.h>
#include <TlHelp32.h>

using std::cout;
using std::cin;

bool improveSleepAcc(bool activate = true);
bool sleep(DWORD ms);
DWORD getPID(char* name);
bool inject(DWORD procId, char* dllPath, HMODULE *hMyDll);
bool unload(DWORD procId, HMODULE hMyDll);

int main()
{
	DWORD skypePID = 0;
	HMODULE hMyDll = 0;

	while ((skypePID = getPID("Skype.exe")) == 0)
	{
		cout << "I don't see Skype.exe\n";

		if (!sleep(100))
		{
			cout << "Cannae sleep\n";
			cin.get();
			return 0;
		}

		system("cls"); // Don't try this @ home
	}

	cout << "now attempting to inject...\n";

	if (!inject(skypePID,
		"L:\\Users\\Fritz\\Documents\\Visual Studio 2015\\Projects\\RecvDetour\\Release\\RecvDetourDll.dll", &hMyDll))
	{
		cout << "Oh no no\n";
		cin.get();
		return 0;
	}

	cout << "Neat\n";

	unload(skypePID, hMyDll); // Unload the Dll from remote proc

	cout << "Unloaded\n";	
	cin.get();

	return 0;
}

bool improveSleepAcc(bool activate)
{
	TIMECAPS tc;
	MMRESULT mmr;

	// Fill the TIMECAPS structure
	if (timeGetDevCaps(&tc, sizeof(tc)) != MMSYSERR_NOERROR)
		throw std::exception("timeGetDevCaps");

	if (activate)
		mmr = timeBeginPeriod(tc.wPeriodMin); // Do manual labor
	else
		mmr = timeEndPeriod(tc.wPeriodMin);

	if (mmr != TIMERR_NOERROR)
		throw std::exception("time_Period");

	return true;
}

bool sleep(DWORD ms)
{
	if (!improveSleepAcc(true))
		return false;

	Sleep(ms);

	if (!improveSleepAcc(false))
		return false;

	return true;
}

DWORD getPID(char* name)
{
	HANDLE hSnapshot = NULL;
	PROCESSENTRY32 pe32;

	// Snap of all procs
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (!hSnapshot)
		throw std::exception("CreateToolhelp32Snapshot");

	if (!Process32First(hSnapshot, &pe32))
		throw std::exception("Process32First");

	do
	{
		if (!strcmp(pe32.szExeFile, name)) // Oh my gat, they da same
		{
			CloseHandle(hSnapshot);
			return pe32.th32ProcessID;
		}
	} while (Process32Next(hSnapshot, &pe32));

	CloseHandle(hSnapshot);

	return 0;
}

bool inject(DWORD procId, char* dllPath, HMODULE *hMyDll)
{
	HANDLE hProcess = NULL;
	void* llPtr;
	void *remDllPath = NULL;
	int dllPathLen = 0;
	HINSTANCE hKernel32 = 0;
	HANDLE hThread = 0;
	DWORD dwReturn = 0;

	if (!hMyDll)
		throw std::exception("hMyDll bad ptr");

	hKernel32 = GetModuleHandle("KERNEL32");
	
	llPtr = reinterpret_cast<void*>
		(GetProcAddress(hKernel32, "LoadLibraryA"));

	if (!llPtr)
		throw std::exception("GetProcAddress");

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, procId);

	if (!hProcess)
		throw std::exception("OpenProcess");

	dllPathLen = strlen(dllPath);

	remDllPath = VirtualAllocEx(hProcess,
		0, dllPathLen + 1, MEM_COMMIT, PAGE_READWRITE);

	if (!remDllPath)
		throw std::exception("VirtualAllocEx");

	// Is the null-terminate automatic or is it garbage of peace
	if (!WriteProcessMemory(hProcess, reinterpret_cast<LPVOID> (remDllPath),
		reinterpret_cast<LPCVOID> (dllPath), dllPathLen + 1, NULL))
		throw std::exception("WriteProcessMemory");
		

	if ((hThread = CreateRemoteThread(hProcess, 0, 0,
		reinterpret_cast<LPTHREAD_START_ROUTINE> (llPtr),
		remDllPath, 0, 0)) == NULL)
	{
		DWORD code = GetLastError();

		throw std::exception("CreateRemoteThread");
	}

	// Get the stupid module handle so we can call funcs from it
	// GetExitCodeThread's 2nd param returns the thread's return
	// and LoadLibrary conveniently returns the handle to the module
	// wow so amaze
	while (GetExitCodeThread(hThread, &dwReturn))
	{
		if (dwReturn != STILL_ACTIVE)
		{
			*hMyDll = reinterpret_cast<HMODULE> (dwReturn);
			break;
		}

		if (!sleep(20))
			throw std::exception("g sleep");
	}

	// Free string
	VirtualFreeEx(hProcess, remDllPath, dllPathLen + 1, MEM_DECOMMIT);

	// It's ok men
	CloseHandle(hProcess);

	return true;
}

bool unload(DWORD procId, HMODULE hMyDll)
{
	HANDLE hProcess = NULL;
	void* flPtr;
	HINSTANCE hKernel32 = 0;

	hKernel32 = GetModuleHandle("KERNEL32");
	
	flPtr = reinterpret_cast<void*>
		(GetProcAddress(hKernel32, "FreeLibrary"));

	if (!flPtr)
		throw std::exception("GetProcAddress");

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, procId);

	if (!hProcess)
		throw std::exception("OpenProcess");

	if (!CreateRemoteThread(hProcess, 0, 0,
		reinterpret_cast<LPTHREAD_START_ROUTINE> (flPtr),
		hMyDll, 0, 0))
		throw std::exception("CreateRemoteThread");

	// It's ok men
	CloseHandle(hProcess);

	return true;
}