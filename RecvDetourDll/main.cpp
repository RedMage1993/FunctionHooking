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

#pragma comment(lib, "detours.lib")

#include <iostream>
#include <cstdio>
#include <io.h>
#include <fcntl.h>
#include <Windows.h>
#include <WinSock2.h>
#include <detours.h>

using std::ios;
using std::cout;
using std::endl;
using std::printf;

typedef int (__stdcall *fnRecv)(SOCKET s, char *buf, int len, int flags);

int __stdcall dRecv(SOCKET s, char *buf, int len, int flags);

fnRecv rRecv = NULL;
HANDLE hCThread = NULL;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReason, LPVOID lpReserved)
{
	HMODULE hWinsock = NULL;
	int hConHandle;
	HANDLE hStdHandle;
	FILE* fp;
	HMENU hMenu = 0;

	UNREFERENCED_PARAMETER(lpReserved); // So fancy

	switch (ulReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);

		AllocConsole();
		ShowWindow(GetConsoleWindow(), SW_HIDE);

		hMenu = GetSystemMenu(GetConsoleWindow(), FALSE);
		DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);

		hStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		hConHandle = _open_osfhandle(reinterpret_cast<intptr_t> (hStdHandle), _O_TEXT);
		fp = _fdopen(hConHandle, "w");
		*stdout = *fp;
		setvbuf(stdout, NULL, _IONBF, 0);
		ios::sync_with_stdio();

		ShowWindow(GetConsoleWindow(), SW_SHOWNORMAL);

		hCThread = GetCurrentThread();
		hWinsock = GetModuleHandle("WS2_32.dll");
		rRecv = reinterpret_cast<fnRecv> (GetProcAddress(hWinsock, "recv"));

		if (!rRecv)
			return TRUE;

		DetourTransactionBegin();
		DetourUpdateThread(hCThread);
		DetourAttach(&(reinterpret_cast<void*&> (rRecv)), dRecv);
		if (DetourTransactionCommit() == NO_ERROR)
			cout << "we made it\n\n";

		break;

	case DLL_PROCESS_DETACH:
		DetourTransactionBegin();
		DetourUpdateThread(hCThread);
		DetourDetach(&(reinterpret_cast<void*&> (rRecv)), dRecv);
		if (DetourTransactionCommit() == NO_ERROR)
			cout << "we made it again\n\n";

		FreeConsole();

		break;
	}
	

	return TRUE;
}

int __stdcall dRecv(SOCKET s, char *buf, int len, int flags)
{
	int rd = rRecv(s, buf, len, flags);
	char hexa[] = {'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	BYTE *k = reinterpret_cast<BYTE*> (buf);

	cout << "size: " << len << endl;
	for (int i = 0; i < len;)
	{
		putchar(hexa[k[i] >> 4]);
		putchar(hexa[k[i] & 0x0F]);
		putchar(' ');

		if (++i % 20 == 0)
			putchar('\n');
	}

	cout << endl << endl;

	return rd;
}