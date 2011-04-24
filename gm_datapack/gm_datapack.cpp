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

#include "GMLuaModule.h"
#include "CProcessModule.h"
#include "CFunctionHook.h"

GMOD_MODULE(Initialize, Shutdown);
ILuaInterface *g_Lua;

#define GM_DATAPACK_VERSION "1.0"
#define COMPRESSFILES_SIGNATURE "83EC44538BD98B43 14895C241085C00F8E430400008B5308 55568BEA57C7442410080000002BEA8B F88DA424000000008B042A8D70018A08 4084C975F98B0A8B89140100002BC68B 74241003C18D4406"

q3k::CFunctionHook<int (*)(char *)> *g_CompressFilesHook;
// This is a hack to have a __thiscall function with ecx as this.
class UnknownClass {
	public:
		int CompressFilesHook(char *Path)
		{
			unsigned int ReturnValue = (this->*CompressFiles)(Path);
			g_Lua->Push(g_Lua->GetGlobal("hook")->GetMember("Call"));
				g_Lua->Push("DatapackCompressed");
				g_Lua->PushNil();
				g_Lua->Push(Path);
			g_Lua->Call(3, 0);
			return ReturnValue;
		}
		static int (UnknownClass:: *CompressFiles)(char *);
};
int (UnknownClass:: *UnknownClass::CompressFiles)(char *Something) = 0;

bool g_HookCreated;

int Initialize(lua_State *L)
{
	g_Lua = Lua();
	g_Lua->Msg("[DataPack] Version " GM_DATAPACK_VERSION " by q3k.org.\n");
	g_HookCreated = false;
	q3k::CProcessModule ServerModule("server");
	
	if (ServerModule.GetBase() == 0)
	{
		g_Lua->Msg("[DataPack] Could not find server module! Module is NOT ACTIVE.\n");
		return 0;
	}

	g_Lua->Msg("[DataPack] Server module @0x%08x, 0x%x bytes.\n", ServerModule.GetBase(), ServerModule.GetSize());

	unsigned int CompressFiles = ServerModule.SignatureScan(COMPRESSFILES_SIGNATURE);
	if (CompressFiles == 0)
	{
		g_Lua->Msg("[DataPack] Could not find the CompressFiles function! Module is NOT ACTIVE.\n");
		return 0;
	}

	g_Lua->Msg("[DataPack] CompressFiles @0x%08x.\n", CompressFiles);
	g_CompressFilesHook = new q3k::CFunctionHook<int (*)(char *)>((int (*)(char *))CompressFiles, (int (*)(char *))(&(unsigned int&)UnknownClass::CompressFilesHook), 9);
	bool Result = g_CompressFilesHook->Hook();

	if (!Result)
	{
		g_Lua->Msg("[DataPack] Could not hook the CompressFiles function! Module is NOT ACTIVE.\n");
		return 0;
	}
	*(void **)(&UnknownClass::CompressFiles) = g_CompressFilesHook->OriginalFunctionTrampoline();

	g_Lua->Msg("[DataPack] Successfully initialized!\n");
	g_HookCreated = true;

	return 0;
}

int Shutdown(lua_State *L)
{
	if (g_HookCreated)
		delete g_CompressFilesHook;
	return 0;
}