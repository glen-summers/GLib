#pragma once

#include "Address.h"

#include "GLib/Win/Debugger.h"

#include <regex>

class Function;

class Coverage : public GLib::Win::Debugger
{
	static constexpr unsigned char debugBreakByte = 0xCC;
	static constexpr unsigned int FooFoo = 0xf00f00;
	static constexpr unsigned int FeeFee = 0xfeefee;

	std::regex nameSpaceRegex{ R"(^(?:[A-Za-z_][A-Za-z_0-9]*::)*)" }; // +some extra unicode chars?

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

	void OnCreateProcess(DWORD processId, DWORD threadId, const CREATE_PROCESS_DEBUG_INFO & info) override;
	void OnExitProcess(DWORD processId, DWORD threadId, const EXIT_PROCESS_DEBUG_INFO& info) override;
	void OnCreateThread(DWORD processId, DWORD threadId, const CREATE_THREAD_DEBUG_INFO & info) override;
	void OnExitThread(DWORD processId, DWORD threadId, const EXIT_THREAD_DEBUG_INFO & info) override;
	DWORD OnException(DWORD processId, DWORD threadId, const EXCEPTION_DEBUG_INFO & info) override;

	static void Delaminate(std::string & name);
	void CleanupFunctionNames(const std::string & name, const std::string & typeName,
		std::string & className, std::string & functionName, std::string & nameSpace) const;

	void CreateHtmlReport(const std::map<ULONG, Function> & indexToFunctionMap, const std::string & title) const;
	CoverageData ConvertFunctionData(const std::map<ULONG, Function> & indexToFunctionMap) const;
};
