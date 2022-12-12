#include "pch.h"

#include "Coverage.h"
#include "FileCoverageData.h"
#include "HtmlReport.h"

#include <GLib/Flogging.h>

#include <iostream>
#include <span>

using namespace std::string_literals;

int main(int const argc, char const * argv[])
{
	int errorCode {};
	std::string_view errorMessage;
	auto const & log = GLib::Flog::LogManager::GetLog("Main");

	try
	{
		std::string_view const desc {"Generates C++ HTML code coverage report"};
		std::string_view const syntax {"Coverage <Executable> <Report> [-sub] [-ws] [-i IncludePath]... [-x excludePath]..."};
		std::string_view const detail {R"(
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
			return 0;
		}

		std::span const args {argv, static_cast<size_t>(argc)};

		auto iter = args.begin();
		auto const end = args.end();
		++iter;
		std::string const & executable = *iter++;
		std::string const & reportPath = *iter++;
		bool debugChildProcesses {};
		bool showWhiteSpace {};

		Strings includes;
		Strings excludes;
		while (iter != end)
		{
			auto const * const arg = *iter++;
			if (strcmp(arg, "-i") == 0)
			{
				if (iter == end)
				{
					throw std::runtime_error("Missing include value");
				}
				includes.insert(*iter++);
			}
			else if (strcmp(arg, "-x") == 0)
			{
				if (iter == end)
				{
					throw std::runtime_error("Missing exclude value");
				}
				excludes.insert(*iter++);
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

		GLib::Flog::ScopeLog const scopeLog {log, GLib::Flog::Level::Info, "Total"};

		Coverage dbg(executable, debugChildProcesses, includes, excludes);
		constexpr unsigned timeoutMilliseconds = 1000;
		while (dbg.ProcessEvents(timeoutMilliseconds))
		{}

		HtmlReport const report(executable, reportPath, dbg.GetCoverageData(), showWhiteSpace);
		static_cast<void>(report);
		static_cast<void>(scopeLog);
	}
	catch (std::exception const & e)
	{
		errorCode = 1;
		errorMessage = e.what();
	}

	log.Error("Error : {0} : {1}", errorCode, errorMessage);
	std::cout << "\x1b[31m\x1b[1m" << errorMessage << "\x1b[m" << std::endl;

	return errorCode;
}
