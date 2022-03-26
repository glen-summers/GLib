#include "pch.h"

#include "Coverage.h"
#include "FileCoverageData.h"
#include "HtmlReport.h"

#include <GLib/Flogging.h>

#include <iostream>
#include <span>

using namespace std::string_literals;

int main(int argc, char * argv[]) // NOLINT(bugprone-exception-escape) potential exception from catch block
{
	auto log = GLib::Flog::LogManager::GetLog("Main");
	int errorCode = 0;
	std::string errorMessage;

	try
	{
		const auto * const desc {"Generates C++ HTML code coverage report"};
		const auto * const syntax {"Coverage <Executable> <Report> [-sub] [-ws] [-i IncludePath]... [-x excludePath]..."};
		const auto * const detail {R"(
Executable: Path to executable
Report    : Directory path for the generated report
[-sub]    : Generates coverage for sub processes of main executable
[-ws]     : Shows visible whitespace in source output
[-i]      : list of source code paths to include
[-x]      : list of source code paths to exclude

Examples:
Coverage c:\Build\Main.exe C:\Report
Coverage c:\Build\Main.exe C:\Report -ws -i C:\MainCode C:\Utils\ -x C:\ExternalCode\
)"};

		if (argc - 1 < 2)
		{
			std::cout << "\x1b[32m\x1b[1m" << desc << std::endl << "\x1b[33m\x1b[1m" << syntax << std::endl << detail << "\x1b[m" << std::endl;
			return errorCode;
		}

		std::span const args {argv, static_cast<size_t>(argc)};

		auto it = args.begin();
		auto end = args.end();
		++it;
		const char * const executable = *it++;
		const char * const reportPath = *it++;
		bool debugChildProcesses {};
		bool showWhiteSpace {};

		Strings includes;
		Strings excludes;
		while (it != end)
		{
			const auto * const arg = *it++;
			if (strcmp(arg, "-i") == 0)
			{
				if (it == end)
				{
					throw std::runtime_error("Missing include value");
				}
				includes.insert(*it++);
			}
			else if (strcmp(arg, "-x") == 0)
			{
				if (it == end)
				{
					throw std::runtime_error("Missing exclude value");
				}
				excludes.insert(*it++);
			}
			else if (strcmp(arg, "-sub") == 0)
			{
				debugChildProcesses = true;
			}
			else if (strcmp(arg, "-ws") == 0)
			{
				showWhiteSpace = true;
			}
			else
			{
				throw std::runtime_error("Unexpected: "s + arg);
			}
		}

		GLib::Flog::ScopeLog scopeLog {log, GLib::Flog::Level::Info, "Total"};

		Coverage dbg(executable, debugChildProcesses, includes, excludes);
		constexpr unsigned timeoutMilliseconds = 1000;
		while (dbg.ProcessEvents(timeoutMilliseconds))
		{}

		HtmlReport report(executable, reportPath, dbg.GetCoverageData(), showWhiteSpace);
		static_cast<void>(report);
		static_cast<void>(scopeLog);
	}
	catch (const std::exception & e)
	{
		errorCode = 1;
		errorMessage = e.what();
	}

	log.Error("Error : {0} : {1}", errorCode, errorMessage);
	std::cout << "\x1b[31m\x1b[1m" << errorMessage << "\x1b[m" << std::endl;

	return errorCode;
}
