#include <vector>
#include <iostream>
#include <windows.h>
#include <psapi.h>
#include "native_debug.h"

using namespace native;

static std::string Wc2Mb(const wchar_t *from)
{
	int size = ::WideCharToMultiByte(CP_ACP, 0, from, -1, nullptr, 0, nullptr, nullptr);
	std::string to;
	if (size > 0) {
		to.resize(size);
		::WideCharToMultiByte(CP_ACP, 0, from, -1, &to.front(), size, nullptr, nullptr);
	}
	return to;
}

static std::wstring Mb2Wc(const char *from)
{
	int size = ::MultiByteToWideChar(CP_UTF8, 0, from, -1, nullptr, 0);
	std::wstring to;
	if (size > 0) {
		to.resize(size);
		::MultiByteToWideChar(CP_UTF8, 0, from, -1, &to.front(), size);
	}
	return to;
}

static std::string MakeErrorMessage(const char *message = nullptr)
{
	LPTSTR msg_buf;
	if (!::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPTSTR>(&msg_buf), 0, nullptr)) {
		throw DebugError("FormatMessage at ThrowDebugError");
	}
	std::string str = Wc2Mb(msg_buf);
	if (::LocalFree(msg_buf)) {
		throw DebugError("LocalFree at ThrowDebugError");
	}

	if (message != nullptr) {
		std::string mes_str(message);
		mes_str.append(":");
		mes_str.append(str);
		return mes_str;
	}
	else {
		return str;
	}
}

RootDebuggee::ProcessManager::ProcessManager()
{
}

RootDebuggee::ProcessManager::~ProcessManager()
{
}

ProcessId RootDebuggee::ProcessManager::GetProcessId()
{
	return process_id_;
}

RootDebuggee::ProcessCreator::ProcessCreator(const char *cmd)
{
	std::wstring cmd_arg = Mb2Wc(cmd);
	OpenInternal(&cmd_arg[0]);
}

RootDebuggee::ProcessCreator::ProcessCreator(const wchar_t *cmd)
{
	std::wstring cmd_arg(cmd);
	OpenInternal(&cmd_arg[0]);
}

void RootDebuggee::ProcessCreator::Close()
{
}

void RootDebuggee::ProcessCreator::OpenInternal(wchar_t *cmd)
{
	PROCESS_INFORMATION pi;
	::ZeroMemory(&pi, sizeof(pi));

	STARTUPINFO si;
	::ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	if (!::CreateProcess(nullptr, cmd, nullptr, nullptr, FALSE, DEBUG_PROCESS | CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi)) {
		throw DebugError(MakeErrorMessage("CreateProcess"));
	}
	if (!::CloseHandle(pi.hThread)) {
		throw DebugError(MakeErrorMessage("CloseHandle thread"));
	}
	if (!::CloseHandle(pi.hProcess)) {
		throw DebugError(MakeErrorMessage("CloseHandle process"));
	}

	process_id_ = pi.dwProcessId;
}

RootDebuggee::ProcessAttacher::ProcessAttacher(ProcessId process_id)
{
	if (!::DebugActiveProcess(process_id)) {
		throw DebugError(MakeErrorMessage("DebugActiveProcess"));
	}
	process_id_ = process_id;
}

void RootDebuggee::ProcessAttacher::Close()
{
	if (!::DebugActiveProcessStop(process_id_)) {
		throw DebugError(MakeErrorMessage("DebugActiveProcessStop"));
	}
}

NativeDebuggee::NativeDebuggee(ProcessId process_id) : process_id_(process_id)
{
}

NativeDebuggee::NativeDebuggee(ProcessId process_id, ThreadId thread_id, HANDLE handle_process, HANDLE handle_thread) : process_id_(process_id), thread_id_(thread_id), handle_process_(handle_process), handle_thread_(handle_thread)
{
}

NativeDebuggee::~NativeDebuggee()
{
}

void NativeDebuggee::Init(ThreadId thread_id, HANDLE handle_process, HANDLE handle_thread)
{
	thread_id_ = thread_id;
	handle_process_ = handle_process;
	handle_thread_ = handle_thread;
}

ProcessId NativeDebuggee::GetProcessId()
{
	return process_id_;
}

ThreadId NativeDebuggee::GetThreadId()
{
	return thread_id_;
}

std::size_t NativeDebuggee::ReadMemoryRaw(void *from, void *to, std::size_t size)
{
	SIZE_T read_bytes = 0;
	::ReadProcessMemory(handle_process_, from, to, size, &read_bytes);
	return read_bytes;
}

std::size_t NativeDebuggee::ReadMemoryRaw(MemoryBinary *from, void *to)
{
	return ReadMemoryRaw(from->ptr, to, from->size);
}

void NativeDebuggee::ReadMemory(void *from, BinaryVector *to, std::size_t size)
{
	SIZE_T read_bytes = 0;
	to->resize(size);
	if (!::ReadProcessMemory(handle_process_, from, &to->front(), size, &read_bytes)) {
		if (!read_bytes) {
			to->clear();
			return;
		}
	}
	to->resize(read_bytes);
	return;
}

void NativeDebuggee::ReadMemory(MemoryBinary *from, BinaryVector *to)
{
	ReadMemory(from->ptr, to, from->size);
}

void NativeDebuggee::ReadMemoryString(void *from, std::string *to, std::size_t size, bool is_unicode)
{
	SIZE_T read_bytes = 0;
	to->resize(size);
	if (!::ReadProcessMemory(handle_process_, from, &to->front(), size, &read_bytes)) {
		if (!read_bytes) {
			to->clear();
			return;
		}
	}
	if (!is_unicode) {
		to->resize(read_bytes);
	}
	else {
		*to = Wc2Mb(reinterpret_cast<wchar_t*>(&to->front()));
	}
	return;
}

void NativeDebuggee::ReadMemoryString(MemoryString *from, std::string *to)
{
	ReadMemoryString(from->ptr, to, from->size, from->is_unicode);
}

void NativeDebuggee::GetRegister(RegisterInfo *reg)
{
	CONTEXT ctx = { CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS };
	if (!::GetThreadContext(handle_thread_, &ctx)) {
		throw DebugError(MakeErrorMessage("GetThreadContext"));
	}
#if BOOST_ARCH_X86_32
	reg->eax = ctx.Eax;
	reg->ebx = ctx.Ebx;
	reg->ecx = ctx.Ecx;
	reg->edx = ctx.Edx;
	reg->esi = ctx.Esi;
	reg->edi = ctx.Edi;
	reg->esp = ctx.Esp;
	reg->ebp = ctx.Ebp;
	reg->eip = ctx.Eip;
	reg->ss = ctx.SegSs;
	reg->cs = ctx.SegCs;
	reg->ds = ctx.SegDs;
	reg->es = ctx.SegEs;
	reg->fs = ctx.SegFs;
	reg->gs = ctx.SegGs;
	reg->dr[0] = ctx.Dr0;
	reg->dr[1] = ctx.Dr1;
	reg->dr[2] = ctx.Dr2;
	reg->dr[3] = ctx.Dr3;
	reg->dr[6] = ctx.Dr6;
	reg->dr[7] = ctx.Dr7;
#elif BOOST_ARCH_X86_64
	reg->rax = ctx.Rax;
	reg->rbx = ctx.Rbx;
	reg->rcx = ctx.Rcx;
	reg->rdx = ctx.Rdx;
	reg->rsi = ctx.Rsi;
	reg->rdi = ctx.Rdi;
	reg->rsp = ctx.Rsp;
	reg->rbp = ctx.Rbp;
	reg->rip = ctx.Rip;
	reg->ss = ctx.SegSs;
	reg->cs = ctx.SegCs;
	reg->ds = ctx.SegDs;
	reg->es = ctx.SegEs;
	reg->fs = ctx.SegFs;
	reg->gs = ctx.SegGs;
	reg->dr[0] = ctx.Dr0;
	reg->dr[1] = ctx.Dr1;
	reg->dr[2] = ctx.Dr2;
	reg->dr[3] = ctx.Dr3;
	reg->dr[6] = ctx.Dr6;
	reg->dr[7] = ctx.Dr7;
#endif
}

void NativeDebuggee::SetSingleStep(bool enable)
{
	if (is_single_step_ != enable) {
		is_single_step_ = enable;
		OnSingleStepChanged();
	}
}

void NativeDebuggee::OnDebugEvent()
{
	if (is_single_step_) {
		OnSingleStepChanged();
	}
}

void NativeDebuggee::OnSingleStepChanged()
{
	CONTEXT ctx = { CONTEXT_CONTROL };
	if (!::GetThreadContext(handle_thread_, &ctx)) {
		throw DebugError(MakeErrorMessage("GetThreadContext"));
	}
	if (is_single_step_) {
		ctx.EFlags |= static_cast<DWORD>(RegisterInfo::Eflag::kTF);
	}
	else {
		ctx.EFlags &= ~static_cast<DWORD>(RegisterInfo::Eflag::kTF);
	}
	if (!::SetThreadContext(handle_thread_, &ctx)) {
		throw DebugError(MakeErrorMessage("SetThreadContext"));
	}
}

RootDebuggee::RootDebuggee(const char *cmd) : RootDebuggee(std::move(std::unique_ptr<ProcessManager>(new ProcessCreator(cmd))))
{
}

RootDebuggee::RootDebuggee(const wchar_t *cmd) : RootDebuggee(std::move(std::unique_ptr<ProcessManager>(new ProcessCreator(cmd))))
{
}

RootDebuggee::RootDebuggee(ProcessId process_id) : RootDebuggee(std::move(std::unique_ptr<ProcessManager>(new ProcessAttacher(process_id))))
{
}

RootDebuggee::RootDebuggee(std::unique_ptr<ProcessManager> &&process) : process_(std::move(process))
{
}

RootDebuggee::~RootDebuggee()
{
	process_->Close();
}

void RootDebuggee::Init(HANDLE handle_process)
{
	handle_process_ = handle_process;
}

ProcessId RootDebuggee::GetProcessId()
{
	return process_->GetProcessId();
}

NativeDebuggee* RootDebuggee::New(ThreadId thread_id, HANDLE handle_thread)
{
	auto child = std::unique_ptr<NativeDebuggee>(new NativeDebuggee(process_->GetProcessId(), thread_id, handle_process_, handle_thread));
	auto it = childs_.insert(std::make_pair(thread_id, std::move(child)));
	if (!it.second) {
		throw DebugError("Child already inserted");
	}
	return it.first->second.get();
}

void RootDebuggee::Delete(ThreadId thread_id)
{
	if (childs_.erase(thread_id) != 1) {
		throw DebugError("Deleting child failed");
	}
}

NativeDebuggee* RootDebuggee::Get(ThreadId thread_id)
{
	auto it = childs_.find(thread_id);
	if (it == std::end(childs_)) {
		throw DebugError("Getting child failed");
	}
	return it->second.get();
}

bool RootDebuggee::IsEmpty()
{
	return childs_.empty();
}

NativeDebugger::NativeDebugger()
{
	HANDLE hToken;
	if (!::OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ, &hToken)) {
		throw DebugError(MakeErrorMessage("OpenProcessToken"));
	}

	LUID luid;
	if (!::LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &luid)) {
		throw DebugError(MakeErrorMessage("LookupPrivilegeValue"));
	}

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!::AdjustTokenPrivileges(hToken, FALSE, &tp, 0, 0, 0)) {
		throw DebugError(MakeErrorMessage("AdjustTokenPrivileges"));
	}

	if (!::CloseHandle(hToken)) {
		throw DebugError(MakeErrorMessage("CloseHandle privileges"));
	}
}

NativeDebugger::~NativeDebugger()
{
	DetachAll();
}

DebugInfo* NativeDebugger::WaitDebugger()
{
	if (!::WaitForDebugEvent(&de_, INFINITE)) {
		throw DebugError(MakeErrorMessage("WaitForDebugEvent"));
	}

	event_info.root_debuggee = debuggees_.at(de_.dwProcessId).get();

	switch (de_.dwDebugEventCode) {
	case CREATE_PROCESS_DEBUG_EVENT:
		event_info.root_debuggee->Init(de_.u.CreateProcessInfo.hProcess);
		event_info.debuggee = event_info.root_debuggee->New(de_.dwThreadId, de_.u.CreateProcessInfo.hThread);
	break;
	case CREATE_THREAD_DEBUG_EVENT:
		event_info.debuggee = debuggees_.at(de_.dwProcessId)->New(de_.dwThreadId, de_.u.CreateThread.hThread);
		break;
	default:
		event_info.debuggee = debuggees_.at(de_.dwProcessId)->Get(de_.dwThreadId);
		break;
	}

	switch (de_.dwDebugEventCode) {
		using EventCode = DebugInfo::EventCode;

	case CREATE_PROCESS_DEBUG_EVENT:
		event_info.code = EventCode::kCreateProcess;
		break;
	case CREATE_THREAD_DEBUG_EVENT:
		event_info.code = EventCode::kCreateThread;
		break;
	case EXIT_PROCESS_DEBUG_EVENT:
		event_info.code = EventCode::kExitProcess;
		event_info.info.exit_process.exit_code = de_.u.ExitProcess.dwExitCode;
		break;
	case EXIT_THREAD_DEBUG_EVENT:
		event_info.code = EventCode::kExitThread;
		event_info.info.exit_thread.exit_code = de_.u.ExitThread.dwExitCode;
		break;
	case LOAD_DLL_DEBUG_EVENT:
		event_info.code = EventCode::kLoadLib;
		break;
	case UNLOAD_DLL_DEBUG_EVENT:
		event_info.code = EventCode::kUnloadLib;
		break;
	case OUTPUT_DEBUG_STRING_EVENT:
		event_info.code = EventCode::kOutputString;
		event_info.info.output_string.str.ptr = de_.u.DebugString.lpDebugStringData;
		event_info.info.output_string.str.size = de_.u.DebugString.nDebugStringLength;
		event_info.info.output_string.str.is_unicode = (de_.u.DebugString.fUnicode) ? true : false;
		break;
	case EXCEPTION_DEBUG_EVENT:
		switch (de_.u.Exception.ExceptionRecord.ExceptionCode) {
		case EXCEPTION_BREAKPOINT:
			event_info.code = EventCode::kBreakPoint;
			event_info.info.break_point.program_ptr = de_.u.Exception.ExceptionRecord.ExceptionAddress;
			break;
		case EXCEPTION_SINGLE_STEP:
			event_info.code = EventCode::kSingleStep;
			event_info.info.single_step.program_ptr = de_.u.Exception.ExceptionRecord.ExceptionAddress;
			break;
		default:
			event_info.code = EventCode::kException;
			event_info.info.exception.program_ptr = de_.u.Exception.ExceptionRecord.ExceptionAddress;
			break;
		}
		break;
	default:
		event_info.code = EventCode::kUnknown;
		break;
	}

	return &event_info;
}

void NativeDebugger::ContinueDebugger(bool exception_handled)
{
	switch (de_.dwDebugEventCode) {
	case EXIT_PROCESS_DEBUG_EVENT:
		debuggees_.at(de_.dwProcessId)->Delete(de_.dwThreadId);
		if (!event_info.root_debuggee->IsEmpty()) {
			throw DebugError("Debuggee is not empty");
		}
		if (debuggees_.erase(de_.dwProcessId) != 1) {
			throw DebugError("Deleting root failed");
		}
		break;
	case EXIT_THREAD_DEBUG_EVENT:
		debuggees_.at(de_.dwProcessId)->Delete(de_.dwThreadId);
		break;
	default:
		event_info.debuggee->OnDebugEvent();
		break;
	}

	DWORD continue_status = (exception_handled) ? DBG_CONTINUE : DBG_EXCEPTION_NOT_HANDLED;
	if (!::ContinueDebugEvent(de_.dwProcessId, de_.dwThreadId, continue_status)) {
		throw DebugError(MakeErrorMessage("ContinueDebugEvent"));
	}
}

bool NativeDebugger::IsExited()
{
	return debuggees_.empty();
}

void NativeDebugger::RunAndAttach(const char *cmd)
{
	AttachInternal(cmd);
}

void NativeDebugger::RunAndAttach(const wchar_t *cmd)
{
	AttachInternal(cmd);
}

void NativeDebugger::Attach(ProcessId process_id)
{
	AttachInternal(process_id);
}

void NativeDebugger::DetachAll()
{
	debuggees_.clear();
}

template <typename T>
void NativeDebugger::AttachInternal(T arg)
{
	auto debuggee = std::unique_ptr<RootDebuggee>(new RootDebuggee(arg));
	if (!debuggees_.insert(std::make_pair(debuggee->GetProcessId(), std::move(debuggee))).second) {
		throw DebugError("Debuggee already inserted");
	}
}
