#pragma once

#include "Address.h"
#include "Process.h"

#include <GLib/Win/Debugger.h>
#include <GLib/flogging.h>

#include <regex>

class Coverage : public GLib::Win::Debugger
{
	inline static auto const log = GLib::Flog::LogManager::GetLog<Coverage>();

	static constexpr unsigned char debugBreakByte = 0xCC;
	static constexpr unsigned int FooFoo = 0xf00f00;
	static constexpr unsigned int FeeFee = 0xfeefee;

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
		, includes(a2w(includes))
		, excludes(a2w(excludes))
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

	CoverageData GetCoverageData() const;

private:
	static WideStrings a2w(const Strings & strings);
	void AddLine(const std::wstring & fileName, unsigned lineNumber, const GLib::Win::Symbols::SymProcess & symProcess, DWORD64 address,
							 Process & process);

	void OnCreateProcess(DWORD processId, DWORD threadId, const CREATE_PROCESS_DEBUG_INFO & info) override;
	void OnExitProcess(DWORD processId, DWORD threadId, const EXIT_PROCESS_DEBUG_INFO & info) override;
	void OnCreateThread(DWORD processId, DWORD threadId, const CREATE_THREAD_DEBUG_INFO & info) override;
	void OnExitThread(DWORD processId, DWORD threadId, const EXIT_THREAD_DEBUG_INFO & info) override;
	DWORD OnException(DWORD processId, DWORD threadId, const EXCEPTION_DEBUG_INFO & info) override;

	void CleanupFunctionNames(const std::string & name, const std::string & typeName, std::string & nameSpace, std::string & className,
														std::string & functionName) const;
	void CaptureData(DWORD processId);
};
