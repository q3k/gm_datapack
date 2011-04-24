// A cross-platform abstraction of a module / dl library

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
#include <string>

#ifdef _WIN32
#include <Windows.h>
#include <TlHelp32.h>
#elif _LINUX
#error "Not implemented yet!"
#else
#error "Not implemented yet!"
#endif

namespace q3k {
	class CProcessModule {
		public:
			CProcessModule(std::string ModuleName);
			const unsigned int GetBase(void) const;
			const unsigned int GetSize(void) const;
			const unsigned int SignatureScan(std::string Pattern) const;
		private:
			std::string m_ModuleName;
			unsigned int m_ModuleBase;
			unsigned int m_ModuleSize;

			// This is used to find the length of a pattern...
			const unsigned int GetSignatureLength(std::string Pattern) const;
	};
};