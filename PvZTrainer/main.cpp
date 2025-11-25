#include <iostream>
#include <stdlib.h>

#include "debugger.h"
#include "Utils.h"

SModule*	g_Module;
Debugger*	g_Debugger;
bool		g_Loop;
bool		g_Delay;

struct Offsets
{
	DWORD dwBase = 0x331C50;
	DWORD dwSunFlowersBase = 0x868;
	DWORD dwCoinsBase = 0x94C;

	DWORD dwSunFlowers = 0x5578;
	DWORD dwCoins = 0x54;
	DWORD dwTree = 0x120;
} offsets;

struct Values
{
	DWORD pBase;
	DWORD pSunFlowersBase;
	DWORD pCoinsBase;
} data;

inline const void ov_asm_delay()
{
	g_Debugger->Write<DWORD>(g_Module->dwBase + 0x958B6, 0x50497F80);
}

inline const void ov_asm_awake()
{
	g_Debugger->Write<DWORD>(g_Module->dwBase + 0x958B6, 0x00497F80);
}

int main()
{
	char pname[16] = "popcapgame1.exe";

	g_Debugger = new Debugger();

	do
	{
		Sleep(1000);
	} while (!g_Debugger->Attach(pname));
	
	SModule module = g_Debugger->GetModule(pname);
	g_Module = &module;

	std::cout << "-- Trainer --" << std::endl;
	std::cout << "Process: popcapgame1.exe" << std::endl;
	std::cout << "Operations: " << std::endl;
	std::cout << "1 - Override Sun Flower Amount" << std::endl;
	std::cout << "2 - Override Coins Amount" << std::endl;
	std::cout << "3 - Toggle Delay" << std::endl;
	std::cout << "4 - Set Tree New Height" << std::endl;
	std::cout << "5 - God Mode" << std::endl;
	std::cout << "6 - Tree of Wisdom Codes" << std::endl;
	std::cout << std::endl;

	data.pBase = g_Debugger->Read<DWORD>(g_Module->dwBase + offsets.dwBase);
	data.pSunFlowersBase = g_Debugger->Read<DWORD>(data.pBase + offsets.dwSunFlowersBase);
	data.pCoinsBase = g_Debugger->Read<DWORD>(data.pBase + offsets.dwCoinsBase);

	std::cout << "Pointer Debug: " << std::endl;
	std::cout << " - dwBase - " << std::hex << data.pBase << std::endl;
	std::cout << " - dwSunFlowersBase - " << std::hex << data.pSunFlowersBase << std::endl;
	std::cout << " - dwCoinsBase - " << std::hex << data.pCoinsBase << std::endl;
	std::cout << std::endl;

	std::cout << "Game Initial Values: " << std::endl;
	std::cout << "{Entity: Zombie, HP: 270}" << std::endl;
	std::cout << std::endl;

	int i = NULL;
	int iv = NULL;
	bool opq = false;

	while (true)
	{
		if (!g_Loop)
		{
			std::cout << "Operation: ";
			std::cin >> i;
			switch (i)
			{
			case 1:
				std::cout << "Amount: ";
				std::cin >> iv;

				g_Debugger->Write(data.pSunFlowersBase + offsets.dwSunFlowers, iv);

				std::cout << std::endl;
				break;
			case 2:
				std::cout << "Amount: ";
				std::cin >> iv;

				g_Debugger->Write(data.pCoinsBase + offsets.dwCoins, iv);

				break;
			case 3:
				opq = !opq;
				if (opq) ov_asm_delay(); else ov_asm_awake();
				break;
			case 4:
				std::cout << "Amount: ";
				std::cin >> iv;

				g_Debugger->Write(data.pCoinsBase + offsets.dwTree, iv);
				break;
			case 5:
				system("color 4f");
				std::cout << " God Mode Activated! " << std::endl;
				std::cout << " Disable with (F1) on your keyboard." << std::endl;
				g_Loop = true;
				break;
			case 6:
				std::cout << "[Selector]" << std::endl;
				std::cout << "1 - Daisies" << std::endl;
				std::cout << "2 - Dance" << std::endl;
				std::cout << "3 - Pinata" << std::endl;
				std::cout << "4 - Future" << std::endl;
				std::cout << "5 - Mustache" << std::endl;
				std::cout << "6 - Trickedout" << std::endl;
				std::cout << "7 - Sukhbir" << std::endl;
				std::cin >> iv;
				std::cout << std::endl;

				HWND hwnd = FindWindow(NULL, "Plants vs. Zombies");
				if (hwnd != NULL)
				{
					WINDOWPLACEMENT placement;
					placement.length = sizeof(placement);

					if (!GetWindowPlacement(hwnd, &placement))
						break;

					BOOL minimized = (placement.showCmd & SW_SHOWMINIMIZED) != 0;
					if (!minimized)
					{
						::SetFocus(hwnd);
						::SetActiveWindow(hwnd);
						::SetForegroundWindow(hwnd);
					}
					else
					{
						placement.showCmd = SW_SHOWNORMAL;
						SetWindowPlacement(hwnd, &placement);
					}
					switch (iv)
					{
					case 1: Utils::PressKeys("DAISIES"); break;
					case 2: Utils::PressKeys("DANCE"); break;
					case 3: Utils::PressKeys("PINATA"); break;
					case 4: Utils::PressKeys("FUTURE"); break;
					case 5: Utils::PressKeys("MUSTACHE"); break;
					case 6: Utils::PressKeys("TRICKEDOUT"); break;
					case 7: Utils::PressKeys("SUKHBIR"); break;
					}
				}
				break;
			}
		}
		else
		{
			if (GetAsyncKeyState(VK_F1) & 0x8000)
			{
				g_Loop = false;
			}
			g_Debugger->Write(data.pSunFlowersBase + offsets.dwSunFlowers, 9999);
		}
	}

	delete g_Module;
	delete g_Debugger;
	return 0;
}