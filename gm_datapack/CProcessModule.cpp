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

#include "CProcessModule.h"
using namespace q3k;

#include <cstdio>

#include "GMLuaModule.h"
extern ILuaInterface *g_Lua;

char htoi(const char* ptr, unsigned int length)
{
	unsigned int value = 0;
	char ch = *ptr;
	for (unsigned int i = 0; i < length; i++)
	{
		if (ch >= '0' && ch <= '9')
			value = (value << 4) + (ch - '0');
		else if (ch >= 'A' && ch <= 'F')
			value = (value << 4) + (ch - 'A' + 10);
		else if (ch >= 'a' && ch <= 'f')
			value = (value << 4) + (ch - 'a' + 10);
		ch = *(++ptr);
	}
	return (char)value;
}

CProcessModule::CProcessModule(std::string ModuleName)
{
	m_ModuleName = ModuleName;
	m_ModuleBase = 0;
	m_ModuleSize = 0;

#ifdef _WIN32
	DWORD ProcessID = GetCurrentProcessId();
	HANDLE SnaphotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessID);

	if (SnaphotHandle == INVALID_HANDLE_VALUE)
		return;

	MODULEENTRY32 ModuleEntry;
	ModuleEntry.dwSize = sizeof(MODULEENTRY32);

	if (!Module32First(SnaphotHandle, &ModuleEntry))
		return;

	do
	{
		if (!(m_ModuleName + ".dll").compare(ModuleEntry.szModule))
		{
			m_ModuleBase = (unsigned int)ModuleEntry.modBaseAddr;
			m_ModuleSize = ModuleEntry.modBaseSize;
			break;
		}
	} while (Module32Next(SnaphotHandle, &ModuleEntry));
#endif
}

const unsigned int CProcessModule::GetBase(void) const
{
	return m_ModuleBase;
}

const unsigned int CProcessModule::GetSize(void) const
{
	return m_ModuleSize;
}

const unsigned int CProcessModule::GetSignatureLength(std::string Pattern) const
{
	unsigned int Length = 0;
	for (unsigned int i = 0; i < Pattern.length(); i += 2)
	{
		if (Pattern[i] != '?')
			Length++;
	}

	return Length;
}

const unsigned int CProcessModule::SignatureScan(std::string Pattern) const
{
	// Let's get rid of the whitespace first...
	std::string PatternNoWhitespace;
	for (unsigned int i = 0; i < Pattern.length(); i++)
		if (Pattern[i] != ' ' && Pattern[i] != '\t')
			PatternNoWhitespace += Pattern[i];

	// Unlikely, but still.
	unsigned int SignatureLength = GetSignatureLength(PatternNoWhitespace);
	if (SignatureLength > m_ModuleSize)
		return 0;
	// Iterate through all of the possible positions of the pattern in the binary
	for (unsigned int Position = 0; Position < m_ModuleSize - SignatureLength; Position++)
	{
		// Iterate through all the characters of the pattern
		unsigned int Character;
		for (Character = 0; Character < PatternNoWhitespace.length(); Character += 2)
		{
			if (PatternNoWhitespace[Character] == '?')
				continue;

			char Byte = htoi(PatternNoWhitespace.c_str() + Character, 2);
			char ModuleByte = ((char*)m_ModuleBase)[Position + Character / 2];
			if (ModuleByte != Byte)
				break;
		}
		// Why did the loop end?
		if (Character >= PatternNoWhitespace.length())
			return Position + m_ModuleBase;
	}
	return 0;
}