#include "pch.h"

#include "Coverage.h"
#include "FileCoverageData.h"
#include "HtmlReport.h"

#include <GLib/Span.h>
#include <GLib/flogging.h>

#include <iostream>

using namespace std::string_literals;

int main(int argc, char * argv[])
{
	int errorCode = 0;

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

		GLib::Span<char *> const args {argv, argc};

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

		GLib::Flog::ScopeLog scope {GLib::Flog::LogManager::GetLog("Main"), GLib::Flog::Level::Info, "Total"};

		Coverage dbg(executable, debugChildProcesses, includes, excludes);
		constexpr unsigned TimeoutMilliseconds = 1000;
		while (dbg.ProcessEvents(TimeoutMilliseconds))
		{}

		HtmlReport report(executable, reportPath, dbg.GetCoverageData(), showWhiteSpace);
	}
	catch (const std::exception & e)
	{
		GLib::Flog::LogManager::GetLog("Main").Error(e.what());
		std::cout << "\x1b[31m\x1b[1m" << e.what() << "\x1b[m" << '\n';

		errorCode = 1;
	}
	return errorCode;
}
