#pragma once
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <boost/predef.h>
#include <windows.h>

namespace native {
	using ProcessId = DWORD;
	using ThreadId = DWORD;

	using BinaryVector = std::vector<uint8_t>;

	struct MemoryBinary
	{
		void *ptr;
		std::size_t size;
	};

	struct MemoryString : public MemoryBinary
	{
		bool is_unicode;
	};

	struct RegisterInfo
	{
#if BOOST_ARCH_X86_32
		uint32_t eax, ebx, ecx, edx, esi, edi, esp, ebp, eip;
		uint32_t ss, cs, ds, es, fs, gs;
		uint32_t dr[8];
		enum class Eflag : uint32_t
		{
			kCF = 1 << 0,
			kPF = 1 << 2,
			kAF = 1 << 4,
			kZF = 1 << 6,
			kSF = 1 << 7,
			kTF = 1 << 8,
			kIF = 1 << 9,
			kDF = 1 << 10,
			kOF = 1 << 11,
			kIOPL1 = 1 << 12,
			kIOPL2 = 1 << 13,
			kNT = 1 << 14,
			kRF = 1 << 16,
			kVM = 1 << 17,
			kAC = 1 << 18,
			kVIF = 1 << 19,
			kVIP = 1 << 20,
			kID = 1 << 21,
		} eflags;
#elif BOOST_ARCH_X86_64
		uint64_t rax, rbx, rcx, rdx, rsi, rdi, rsp, rbp, rip;
		uint16_t ss, cs, ds, es, fs, gs;
		uint64_t dr[8];
		enum class Eflag : uint32_t
		{
			kCF = 1 << 0,
			kPF = 1 << 2,
			kAF = 1 << 4,
			kZF = 1 << 6,
			kSF = 1 << 7,
			kTF = 1 << 8,
			kIF = 1 << 9,
			kDF = 1 << 10,
			kOF = 1 << 11,
			kIOPL1 = 1 << 12,
			kIOPL2 = 1 << 13,
			kNT = 1 << 14,
			kRF = 1 << 16,
			kVM = 1 << 17,
			kAC = 1 << 18,
			kVIF = 1 << 19,
			kVIP = 1 << 20,
			kID = 1 << 21,
		} eflags;
#endif
	};

	class NativeDebuggee
	{
	public:
		NativeDebuggee(ProcessId process_id);
		NativeDebuggee(ProcessId process_id, ThreadId thread_id, HANDLE handle_process, HANDLE handle_thread);
		virtual ~NativeDebuggee();

		void Init(ThreadId thread_id, HANDLE handle_process, HANDLE handle_thread);
		ProcessId GetProcessId();
		ThreadId GetThreadId();

		std::size_t ReadMemoryRaw(void *from, void *to, std::size_t size);
		std::size_t ReadMemoryRaw(MemoryBinary *from, void *to);
		void ReadMemory(void *from, BinaryVector *to, std::size_t size);
		void ReadMemory(MemoryBinary *from, BinaryVector *to);
		void ReadMemoryString(void *from, std::string *to, std::size_t size, bool is_unicode);
		void ReadMemoryString(MemoryString *from, std::string *to);

		void GetRegister(RegisterInfo *reg);
		void SetSingleStep(bool enable);
		void OnDebugEvent();

	private:
		void OnSingleStepChanged();

		ProcessId process_id_;
		ThreadId thread_id_;
		HANDLE handle_process_ = nullptr;
		HANDLE handle_thread_ = nullptr;
		bool is_single_step_ = false;
	};

	class RootDebuggee
	{
	public:
		RootDebuggee(const char *cmd);
		RootDebuggee(const wchar_t *cmd);
		RootDebuggee(ProcessId process_id);
		~RootDebuggee();

		void Init(HANDLE handle_process);
		ProcessId GetProcessId();

		NativeDebuggee* New(ThreadId thread_id, HANDLE handle_thread);
		void Delete(ThreadId thread_id);
		NativeDebuggee* Get(ThreadId thread_id);
		bool IsEmpty();

	private:
		class ProcessManager
		{
		public:
			ProcessManager();
			virtual ~ProcessManager();
			virtual void Close() = 0;
			ProcessId GetProcessId();
		protected:
			ProcessId process_id_;
		};

		class ProcessCreator : public ProcessManager
		{
		public:
			ProcessCreator(const char *cmd);
			ProcessCreator(const wchar_t *cmd);
			void Close() override;
		private:
			void OpenInternal(wchar_t *cmd);
		};

		class ProcessAttacher : public ProcessManager
		{
		public:
			ProcessAttacher(ProcessId process_id);
			void Close() override;
		};

		RootDebuggee(std::unique_ptr<ProcessManager> &&process);

		std::unique_ptr<ProcessManager> process_;
		HANDLE handle_process_ = nullptr;

		using ChildMap = std::map<ThreadId, std::unique_ptr<NativeDebuggee>>;
		ChildMap childs_;
	};

	struct DebugInfo
	{
		RootDebuggee *root_debuggee;
		NativeDebuggee *debuggee;

		enum class EventCode
		{
			kUnknown,
			kCreateProcess,
			kCreateThread,
			kExitProcess,
			kExitThread,
			kLoadLib,
			kUnloadLib,
			kOutputString,
			kException,
			kSingleStep,
			kBreakPoint,
		} code;

		union {
			struct {
				int exit_code;
			} exit_process;

			struct {
				int exit_code;
			} exit_thread;

			struct {
				MemoryString str;
			} output_string;

			struct {
				void *program_ptr;
			} exception;

			struct {
				void *program_ptr;
			} single_step;

			struct {
				void *program_ptr;
			} break_point;
		} info;
	};

	class NativeDebugger
	{
	public:
		NativeDebugger();
		~NativeDebugger();

		DebugInfo* WaitDebugger();
		void ContinueDebugger(bool exception_handled = true);
		bool IsExited();

		void RunAndAttach(const char *cmd);
		void RunAndAttach(const wchar_t *cmd);
		void Attach(ProcessId process_id);
		void DetachAll();

	private:
		template <typename T>
		void AttachInternal(T arg);

		DEBUG_EVENT de_;
		DebugInfo event_info;

		using ProcessMap = std::map<ProcessId, std::unique_ptr<RootDebuggee>>;
		ProcessMap debuggees_;
	};

	class DebugError : public std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};
}
