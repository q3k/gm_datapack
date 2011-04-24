// A cross-platform Hook/Detour class by q3k.org

//	  This file is part of gm_datapack.
//
//    gm_datapack is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    gm_datapack is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with gm_datapack. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <cstdio>

#ifdef _WIN32
#include <Windows.h>
#define CFH_PROLOGUE_SIZE (m_PrologueSize)
#define CFH_TRAMPOLINE_SIZE (CFH_PROLOGUE_SIZE + 8)
#elif _LINUX
#error "Unimplemented!"
#else
#error "Unimplemented on your platform!"
#endif

namespace q3k {
	template <class TADDRESS>
	class CFunctionHook {
		public:
			CFunctionHook(TADDRESS OriginalFunction, TADDRESS HookFunction, unsigned int PrologueSize);
			bool Hook(void);
			void Unhook(void);
			~CFunctionHook(void);
			const TADDRESS OriginalFunctionTrampoline(void) const;
		private:
			TADDRESS m_OriginalFunction;
			TADDRESS m_HookFunction;
			TADDRESS m_Trampoline;
			bool m_Hooked;
			unsigned int m_PrologueSize;
#ifdef _WIN32
			DWORD m_OldProtectionValue;
#endif
	};
	
	template <class TADDRESS>
	CFunctionHook<TADDRESS>::CFunctionHook(TADDRESS OriginalFunction, TADDRESS HookFunction, unsigned int PrologueSize)
	{
		m_OriginalFunction = OriginalFunction;
		m_HookFunction = HookFunction;
		m_PrologueSize = PrologueSize;
		m_Trampoline = 0;
		m_Hooked = false;
	}

	template <class TADDRESS>
	CFunctionHook<TADDRESS>::~CFunctionHook(void)
	{
		if (m_Hooked)
			Unhook();
	}

	template <class TADDRESS>
	bool CFunctionHook<TADDRESS>::Hook(void)
	{
		if (CFH_PROLOGUE_SIZE < 8)
			return false;

#ifdef _WIN32
		// Original function:
		//  ________________________           _________________________
		// |                        |         | mov eax, m_HookFunction |
		// |                        |         |         jmp eax         |
		// |                        |         |_________________________|---> m_HookFunction
		// |        PROLOGUE        | hooking | _ _ _ _ _ _ _ _ _ _ _ _ |
		// |                        |  ====>  |_________________________|
		// |                        |         |         pop eax         | <---\
		// |________________________|         |_________________________|      |
		// |          CODE          |         |           CODE          |      |
		// |/\/\/\/\/\/\/\/\/\/\/\/\|         |/\/\/\/\/\/\/\/\/\/\/\/\/|      |
		//                                                                     |
		//                                                                     |
		//  Return trampoline                                                  |
		//  ________________________                                           |
		// |                        | <--- m_HookFunction                      |
		// |                        |                                          |
		// |                        |                                          |
		// |        PROLOGUE        |                                          |
		// |                        |                                          |
		// |                        |                                          |
		// |________________________|                                          |
		// |        push eax        |                                          |
		// | mov eax, StartOfReturn |                                          |
		// |         jmp eax        |                                          |
		// |________________________|------------------------------------------/

		// Change protection on memory block to be overwritten
		BOOL Result = VirtualProtect((LPVOID)m_OriginalFunction, CFH_PROLOGUE_SIZE, PAGE_EXECUTE_READWRITE, &m_OldProtectionValue);
		if (Result == 0)
		{
			printf("[e] %i\n", GetLastError());
			return false;
		}

		// Allocate memory for our trampoline
		unsigned char *Trampoline = (unsigned char*)malloc(CFH_TRAMPOLINE_SIZE * sizeof(char));
		DWORD NotNeededReally;
		VirtualProtect((LPVOID)Trampoline, CFH_TRAMPOLINE_SIZE, PAGE_EXECUTE_READWRITE, &NotNeededReally);

		// Copy over function prologue...
		memcpy(Trampoline, (char*)m_OriginalFunction, CFH_PROLOGUE_SIZE);
	
		unsigned int StartOfReturn = (unsigned int)m_OriginalFunction + CFH_PROLOGUE_SIZE - 1; // We need one byte for a pop eax;

		// Add trampoline code...
		Trampoline[CFH_PROLOGUE_SIZE]     = '\x50';                        // push eax;
		Trampoline[CFH_PROLOGUE_SIZE + 1] = '\xB8';                        // mov eax
		Trampoline[CFH_PROLOGUE_SIZE + 2] = StartOfReturn & 0xFF;          //
		Trampoline[CFH_PROLOGUE_SIZE + 3] = (StartOfReturn >> 8) & 0xFF;   // m_OriginalFunction;
		Trampoline[CFH_PROLOGUE_SIZE + 4] = (StartOfReturn >> 16) & 0xFF;  //
		Trampoline[CFH_PROLOGUE_SIZE + 5] = (StartOfReturn >> 24) & 0xFF;  //
		Trampoline[CFH_PROLOGUE_SIZE + 6] = '\xFF';                        // jmp
		Trampoline[CFH_PROLOGUE_SIZE + 7] = '\xE0';                        //     eax;

		// Add the jmp to m_HookFunction to the beggining of the original function
		char *OriginalFunction = (char*)m_OriginalFunction;
		unsigned int HookFunction = (unsigned int)m_HookFunction;
		OriginalFunction[0] = '\xB8';
		OriginalFunction[1] = HookFunction & 0xFF;
		OriginalFunction[2] = (HookFunction >> 8) & 0xFF;
		OriginalFunction[3] = (HookFunction >> 16) & 0xFF;
		OriginalFunction[4] = (HookFunction >> 24) & 0xFF;
		OriginalFunction[5] = '\xFF';
		OriginalFunction[6] = '\xE0';

		// Add the pop eax right before the end of the old prologue
		OriginalFunction[CFH_PROLOGUE_SIZE - 1] = '\x58';

		m_Trampoline = (TADDRESS)Trampoline;
		m_Hooked = true;

		return true;
#endif
	}

	template <class TADDRESS>
	const TADDRESS CFunctionHook<TADDRESS>::OriginalFunctionTrampoline(void) const
	{
		return m_Trampoline;
	}

	template <class TADDRESS>
	void CFunctionHook<TADDRESS>::Unhook(void)
	{
		if (m_Hooked)
		{
			// Copy the prologue back
			memcpy((char*)m_OriginalFunction, (char*)m_Trampoline, CFH_PROLOGUE_SIZE);

#ifdef _WIN32
			// Set the old protection value on the modified memory
			DWORD NotNeededReally;
			VirtualProtect((LPVOID)m_OriginalFunction, CFH_PROLOGUE_SIZE, m_OldProtectionValue, &NotNeededReally);
#endif
			// Free the trampoline
			free((char*)m_Trampoline);

			m_Hooked = false;
		}
	}
};