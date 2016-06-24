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
			std::cout << "- �v���Z�X�쐬�F" << event_info.debuggee->GetThreadId() << std::endl;
			event_info.debuggee->SetSingleStep(true);
			break;
		case EventCode::kCreateThread:
			std::cout << "- �X���b�h�쐬�F" << event_info.debuggee->GetThreadId() << std::endl;
			//event_info.debuggee->SetSingleStep(true);
			break;
		case EventCode::kExitProcess:
			std::cout << "- �v���Z�X�I���F" << event_info.debuggee->GetThreadId() << std::endl;
			std::cout << "-- �I���R�[�h�F" << event_info.info.exit_process.exit_code << std::endl;
			break;
		case EventCode::kExitThread:
			std::cout << "- �X���b�h�I���F" << event_info.debuggee->GetThreadId() << std::endl;
			std::cout << "-- �I���R�[�h�F" << event_info.info.exit_thread.exit_code << std::endl;
			break;
		case EventCode::kLoadLib:
			std::cout << "- ���C�u�����̃��[�h�F" << event_info.debuggee->GetThreadId() << std::endl;
			break;
		case EventCode::kUnloadLib:
			std::cout << "- ���C�u�����̃A�����[�h�F" << event_info.debuggee->GetThreadId() << std::endl;
			break;
		case EventCode::kOutputString:
			std::cout << "- �f�o�b�O������F" << event_info.debuggee->GetThreadId() << std::endl;
			{
				std::string str;
				event_info.debuggee->ReadMemoryString(&event_info.info.output_string.str, &str);
				std::cout << "-- ������F" << str << std::endl;
			}
			break;
		case EventCode::kException:
			std::cout << "- ��O�����F" << event_info.debuggee->GetThreadId() << std::endl;
			exception_handled = false;
			break;
		case EventCode::kSingleStep:
			//std::cout << "- �V���O���X�e�b�v�E�u���[�N�|�C���g�F" << event_info.debuggee->GetThreadId() << std::endl;
			break;
		case EventCode::kBreakPoint:
			std::cout << "- �u���[�N�|�C���g�F" << event_info.debuggee->GetThreadId() << std::endl;
			break;
		case EventCode::kUnknown:
			std::cout << "- �s���ȃf�o�b�O�C�x���g�F" << event_info.debuggee->GetThreadId() << std::endl;
			break;
		default:
			std::cout << "- �\�����Ȃ��C�x���g" << event_info.debuggee->GetThreadId() << std::endl;
			break;
		}

		std::cout << std::flush;
		debugger_.ContinueDebugger(exception_handled);
	}
}
