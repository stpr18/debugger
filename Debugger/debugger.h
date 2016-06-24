#pragma once
#include <memory>
#include "native_debug.h"

class Debugger
{
public:
	Debugger();
	~Debugger();

	void RunAndAttach(const char *path);
	void RunAndAttach(const wchar_t *path);
	void Attach(native::ProcessId process_id);
	void DetachAll();
	void MainLoop();

private:
	native::NativeDebugger debugger_;
};
