// external-csgo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <chrono>
#include <thread>
#include "offsets.hpp"

HANDLE handle;
DWORD processId;

DWORD client;
DWORD engine;

struct vector3d
{
	float x, y, z;
};

struct variables
{
	struct glow
	{
		bool isEnabled = true;
	}glow;

	struct misc
	{
		bool bhop = true;
	}misc;
}vars;

template<class T>
T read(DWORD addr) {
	T data;
	ReadProcessMemory(handle, (LPVOID)addr, &data, sizeof(T), 0);
	return data;
}

template<class T>
void write(DWORD addr, T data) {
	WriteProcessMemory(handle, (LPVOID)addr, &data, sizeof(T), 0);
}

DWORD getLocalPlayer() {
	return read<DWORD>(client + hazedumper::signatures::dwLocalPlayer);
}

DWORD getEntityFromIndex(int index) {
	return read<int>(client + hazedumper::signatures::dwEntityList + index * 0x10);
}

int getHealth(DWORD entity) {
	return read<int>(entity + hazedumper::netvars::m_iHealth);
}

int getTeam(DWORD entity) {
	return read<int>(entity + hazedumper::netvars::m_iTeamNum);
}

bool isDormat(DWORD entity) {
	return read<bool>(entity + hazedumper::signatures::m_bDormant);
}

bool attachToProc(const char* procName) {
	PROCESSENTRY32 procEntry;
	procEntry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE handleProcId = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	while (Process32Next(handleProcId, &procEntry))
	{
		if (!strcmp(procEntry.szExeFile, procName)) {
			processId = procEntry.th32ProcessID;
			handle = OpenProcess(PROCESS_ALL_ACCESS, 0, processId);
			CloseHandle(handleProcId);
			return true;
		}
	}

	CloseHandle(handleProcId);
	return false;
}

DWORD getModule(const char* name) {
	MODULEENTRY32 entry;
	entry.dwSize = sizeof(MODULEENTRY32);

	HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);

	while (Module32Next(handle, &entry))
	{
		if (!strcmp(entry.szModule, name)) {
			CloseHandle(handle);
			return (DWORD)entry.modBaseAddr;
		}
	}
}

void glow() {
	if (!vars.glow.isEnabled)
		return;

	for (int i = 0; i < 64; i++) {
		auto entity = getEntityFromIndex(i);

		if (entity)
			write<float>(entity + 0x3964, 4500);
	}
}

void bhop() {
	if (!vars.misc.bhop)
		return;

	auto localPlayer = getLocalPlayer();
	auto flags = read<int>(localPlayer + hazedumper::netvars::m_fFlags);

	if (localPlayer && flags == 257 && GetAsyncKeyState(VK_SPACE) & 0x8000) {
		write<int>(client + hazedumper::signatures::dwForceJump, 6);
	}
}

int main()
{
	std::cout << "started...\n";

	if (attachToProc("csgo.exe")) {
		std::cout << "succesfully attached to proc\n";

		client = getModule("client_panorama.dll");
		engine = getModule("engine.dll");

		std::cout << "client: " << client << "\n";
		std::cout << "engine: " << engine << "\n";

		for (;;) {
			glow();
			bhop();

			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}

	std::cout << "couldnt attach to proc\n";

	return 0;
}