#pragma once

#include "Address.h"
#include "HtmlPrinter.h"

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

	void CreateReport(unsigned int processId);

private:
	static WideStrings a2w(const Strings& strings);
	void AddLine(const std::wstring & fileName, unsigned lineNumber, const GLib::Win::Symbols::SymProcess & process, DWORD64 address);
	void CreateXmlReport(const std::map<ULONG, Function> & indexToFunction) const;
	void CreateHtmlReport(const std::map<ULONG, Function> & indexToFunctionMap, const std::string & title) const;
	void GenerateHtmlFile(const std::filesystem::path & sourceFile, const std::filesystem::path & destFile, const std::map<unsigned int, size_t> & lines,
		const std::filesystem::path & css, const std::string & title, unsigned int coveragePercent) const;

	void OnCreateProcess(DWORD processId, DWORD threadId, const CREATE_PROCESS_DEBUG_INFO & info) override;
	void OnExitProcess(DWORD processId, DWORD threadId, const EXIT_PROCESS_DEBUG_INFO& info) override;
	void OnCreateThread(DWORD processId, DWORD threadId, const CREATE_THREAD_DEBUG_INFO & info) override;
	void OnExitThread(DWORD processId, DWORD threadId, const EXIT_THREAD_DEBUG_INFO & info) override;
	DWORD OnException(DWORD processId, DWORD threadId, const EXCEPTION_DEBUG_INFO & info) override;

	static void AddHtmlCoverageBar(HtmlPrinter& printer, unsigned int percent);
};
