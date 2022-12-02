#pragma once

#include "Process.h"

#include <GLib/Flogging.h>
#include <GLib/Win/Debugger.h>

#include <regex>

class Coverage : public GLib::Win::Debugger
{
	inline static auto const log = GLib::Flog::LogManager::GetLog<Coverage>();

	static constexpr unsigned char debugBreakByte = 0xCC;
	static constexpr unsigned int fooFoo = 0xf00f00;
	static constexpr unsigned int feeFee = 0xfeefee;

	std::regex nameSpaceRegex {R"(^(?:[A-Za-z_][A-Za-z_0-9]*::)*)"}; // +some extra unicode chars?

	std::string executable;
	WideStrings includes;
	WideStrings excludes;

	WideStrings wideFiles;
	Processes processes;

public:
	Coverage(std::string const & executable, bool const debugChildProcesses, Strings const & includes, Strings const & excludes)
		: Debugger(executable, debugChildProcesses)
		, executable(executable)
		, includes(A2W(includes))
		, excludes(A2W(excludes))
	{
		GLib::Flog::Detail::Stream() << std::boolalpha;
		log.Info("Executable: {0}, DebugSubProcess: {1}", executable, debugChildProcesses);
		for (auto const & i : includes)
		{
			log.Info("Include: {0}", i);
		}
		for (auto const & x : excludes)
		{
			log.Info("Exclude: {0}", x);
		}
	}

	[[nodiscard]] CoverageData GetCoverageData() const;

private:
	static WideStrings A2W(Strings const & strings);
	void AddLine(std::wstring const & fileName, unsigned int lineNumber, GLib::Win::Symbols::SymProcess const & symProcess, uint64_t address,
							 Process & process);

	void OnCreateProcess(ULONG processId, ULONG threadId, CREATE_PROCESS_DEBUG_INFO const & info) override;
	void OnExitProcess(ULONG processId, ULONG threadId, EXIT_PROCESS_DEBUG_INFO const & info) override;
	void OnCreateThread(ULONG processId, ULONG threadId, CREATE_THREAD_DEBUG_INFO const & info) override;
	void OnExitThread(ULONG processId, ULONG threadId, EXIT_THREAD_DEBUG_INFO const & info) override;
	ULONG OnException(ULONG processId, ULONG threadId, EXCEPTION_DEBUG_INFO const & info) override;

	[[nodiscard]] std::tuple<std::string, std::string, std::string> CleanupFunctionNames(std::string const & name, std::string const & typeName) const;
	void CaptureData(ULONG processId);
};
