#pragma once

#include "Address.h"

#include <GLib/Win/Debugger.h>

class Function;

class Coverage : public GLib::Win::Debugger
{
	static constexpr unsigned char debugBreakByte = 0xCC;

	std::string executable;
	std::string reportPath;
	WideStrings includes;
	WideStrings excludes;

	WideStrings wideFiles;
	Addresses addresses;

	std::map<unsigned int, HANDLE> threads;

public:
	Coverage(const std::string & executable, std::string reportPath, const Strings & includes, const Strings & excludes)
		: Debugger(executable)
		, executable(executable)
		, reportPath(std::move(reportPath))
		, includes(a2w(includes))
		, excludes(a2w(excludes))
	{}

	std::string CreateReport(unsigned int processId);

private:
	static WideStrings a2w(const Strings& strings);
	void AddLine(const std::wstring & fileName, unsigned lineNumber, const GLib::Win::Symbols::SymProcess & process, DWORD64 address);
	std::string CreateMsVcReport(const std::map<ULONG, Function> & indexToFunction) const;
	std::string CreateGCovReport(const std::map<ULONG, Function> & indexToFunction) const;
	std::string CreateCoberturaReport(const std::map<ULONG, Function> & indexToFunction) const;

	void OnCreateProcess(DWORD processId, DWORD threadId, const CREATE_PROCESS_DEBUG_INFO & info) override;
	void OnExitProcess(DWORD processId, DWORD threadId, const EXIT_PROCESS_DEBUG_INFO& info) override;
	void OnCreateThread(DWORD processId, DWORD threadId, const CREATE_THREAD_DEBUG_INFO & info) override;
	void OnExitThread(DWORD processId, DWORD threadId, const EXIT_THREAD_DEBUG_INFO & info) override;
	DWORD OnException(DWORD processId, DWORD threadId, const EXCEPTION_DEBUG_INFO & info) override;
};

