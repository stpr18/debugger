#include <vector>
#include <iostream>
#include "debugger.h"

Debugger::Debugger()
{
}

Debugger::~Debugger()
{
	DetachAll();
}

void Debugger::RunAndAttach(const char *path)
{
	debugger_.RunAndAttach(path);
}

void Debugger::RunAndAttach(const wchar_t *path)
{
	debugger_.RunAndAttach(path);
}

void Debugger::Attach(native::ProcessId process_id)
{
	debugger_.Attach(process_id);
}

void Debugger::DetachAll()
{
	return debugger_.DetachAll();
}

void Debugger::MainLoop()
{
	bool first = false;
	while (!debugger_.IsExited()) {
		native::DebugInfo& event_info = *debugger_.WaitDebugger();

		bool exception_handled = true;

		switch (event_info.code) {
			using EventCode = native::DebugInfo::EventCode;

		case EventCode::kCreateProcess:
			std::cout << "- プロセス作成：" << event_info.debuggee->GetThreadId() << std::endl;
			event_info.debuggee->SetSingleStep(true);
			break;
		case EventCode::kCreateThread:
			std::cout << "- スレッド作成：" << event_info.debuggee->GetThreadId() << std::endl;
			//event_info.debuggee->SetSingleStep(true);
			break;
		case EventCode::kExitProcess:
			std::cout << "- プロセス終了：" << event_info.debuggee->GetThreadId() << std::endl;
			std::cout << "-- 終了コード：" << event_info.info.exit_process.exit_code << std::endl;
			break;
		case EventCode::kExitThread:
			std::cout << "- スレッド終了：" << event_info.debuggee->GetThreadId() << std::endl;
			std::cout << "-- 終了コード：" << event_info.info.exit_thread.exit_code << std::endl;
			break;
		case EventCode::kLoadLib:
			std::cout << "- ライブラリのロード：" << event_info.debuggee->GetThreadId() << std::endl;
			break;
		case EventCode::kUnloadLib:
			std::cout << "- ライブラリのアンロード：" << event_info.debuggee->GetThreadId() << std::endl;
			break;
		case EventCode::kOutputString:
			std::cout << "- デバッグ文字列：" << event_info.debuggee->GetThreadId() << std::endl;
			{
				std::string str;
				event_info.debuggee->ReadMemoryString(&event_info.info.output_string.str, &str);
				std::cout << "-- 文字列：" << str << std::endl;
			}
			break;
		case EventCode::kException:
			std::cout << "- 例外発生：" << event_info.debuggee->GetThreadId() << std::endl;
			exception_handled = false;
			break;
		case EventCode::kSingleStep:
			//std::cout << "- シングルステップ・ブレークポイント：" << event_info.debuggee->GetThreadId() << std::endl;
			break;
		case EventCode::kBreakPoint:
			std::cout << "- ブレークポイント：" << event_info.debuggee->GetThreadId() << std::endl;
			break;
		case EventCode::kUnknown:
			std::cout << "- 不明なデバッグイベント：" << event_info.debuggee->GetThreadId() << std::endl;
			break;
		default:
			std::cout << "- 予期しないイベント" << event_info.debuggee->GetThreadId() << std::endl;
			break;
		}

		std::cout << std::flush;
		debugger_.ContinueDebugger(exception_handled);
	}
}
