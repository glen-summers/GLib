#pragma once

#include "Address.h"
#include "FileCoverageData.h"

#include <GLib/Win/Debugger.h>

class Function;

class Coverage : public GLib::Win::Debugger
{
	static constexpr unsigned char debugBreakByte = 0xCC;

	std::string executable;
	std::filesystem::path reportPath;
	WideStrings includes;
	WideStrings excludes;

	WideStrings wideFiles;
	Addresses addresses;

	std::map<unsigned int, HANDLE> threads;

public:
	Coverage(const std::string & executable, const std::string & reportPath, const Strings & includes, const Strings & excludes)
		: Debugger(executable)
		, executable(executable)
		, reportPath(GLib::Cvt::a2w(reportPath))
		, includes(a2w(includes))
		, excludes(a2w(excludes))
	{}

	void CreateReport(unsigned int processId);

private:
	static WideStrings a2w(const Strings& strings);
	void AddLine(const std::wstring & fileName, unsigned lineNumber, const GLib::Win::Symbols::SymProcess & process, DWORD64 address);
	void CreateXmlReport(const std::map<ULONG, Function> & indexToFunction) const;

	void OnCreateProcess(DWORD processId, DWORD threadId, const CREATE_PROCESS_DEBUG_INFO & info) override;
	void OnExitProcess(DWORD processId, DWORD threadId, const EXIT_PROCESS_DEBUG_INFO& info) override;
	void OnCreateThread(DWORD processId, DWORD threadId, const CREATE_THREAD_DEBUG_INFO & info) override;
	void OnExitThread(DWORD processId, DWORD threadId, const EXIT_THREAD_DEBUG_INFO & info) override;
	DWORD OnException(DWORD processId, DWORD threadId, const EXCEPTION_DEBUG_INFO & info) override;

	void CreateHtmlReport(const std::map<ULONG, Function> & indexToFunctionMap, const std::string & title) const;
	static std::map<std::filesystem::path, FileCoverageData> ConvertFunctionDataToFileData(const std::map<ULONG, Function> & indexToFunctionMap);
};
