#include <cstdlib>
#include <iostream>
#include "debugger.h"

int main()
{
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetBreakAlloc(867);
#endif

	Debugger debugger;
	debugger.RunAndAttach("Debuggee.exe");
	//debugger.Attach(11208);
	debugger.MainLoop();

	std::system("pause");
	return EXIT_SUCCESS;
}
