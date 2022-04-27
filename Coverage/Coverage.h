#pragma once

#include "Process.h"

#include <GLib/Flogging.h>
#include <GLib/Win/Debugger.h>

#include <regex>

class Coverage : public GLib::Win::Debugger
{
	inline static const auto log = GLib::Flog::LogManager::GetLog<Coverage>();

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
	Coverage(const std::string & executable, bool debugChildProcesses, const Strings & includes, const Strings & excludes)
		: Debugger(executable, debugChildProcesses)
		, executable(executable)
		, includes(A2W(includes))
		, excludes(A2W(excludes))
	{
		GLib::Flog::Detail::Stream() << std::boolalpha;
		log.Info("Executable: {0}, DebugSubProcess: {1}", executable, debugChildProcesses);
		for (const auto & i : includes)
		{
			log.Info("Include: {0}", i);
		}
		for (const auto & x : excludes)
		{
			log.Info("Exclude: {0}", x);
		}
	}

	[[nodiscard]] CoverageData GetCoverageData() const;

private:
	static WideStrings A2W(const Strings & strings);
	void AddLine(const std::wstring & fileName, unsigned int lineNumber, const GLib::Win::Symbols::SymProcess & symProcess, uint64_t address,
							 Process & process);

	void OnCreateProcess(ULONG processId, ULONG threadId, const CREATE_PROCESS_DEBUG_INFO & info) override;
	void OnExitProcess(ULONG processId, ULONG threadId, const EXIT_PROCESS_DEBUG_INFO & info) override;
	void OnCreateThread(ULONG processId, ULONG threadId, const CREATE_THREAD_DEBUG_INFO & info) override;
	void OnExitThread(ULONG processId, ULONG threadId, const EXIT_THREAD_DEBUG_INFO & info) override;
	ULONG OnException(ULONG processId, ULONG threadId, const EXCEPTION_DEBUG_INFO & info) override;

	[[nodiscard]] std::tuple<std::string, std::string, std::string> CleanupFunctionNames(const std::string & name, const std::string & typeName) const;
	void CaptureData(ULONG processId);
};
