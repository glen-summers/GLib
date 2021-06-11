#include "pch.h"

#include "Coverage.h"
#include "FileCoverageData.h"
#include "HtmlReport.h"

#include <GLib/Span.h>
#include <GLib/flogging.h>

#include <iostream>

using namespace std::string_literals;

int main(int argc, char *argv[])
{
	int errorCode = 0;

	try
	{
		const auto * const desc {"Generates C++ HTML code coverage report"};
		const auto * const syntax {"Coverage <Executable> <Report> [-sub] [-i IncludePath]... [-x excludePath]..."};
		const auto * const detail {R"(
Executable: Path to executeable
Report    : Directory path for the generated report
[-sub]    : Generates coverage for sub processes of main executable
[-i]      : list of source code paths to include
[-x]      : list of source code paths to exclude

Examples:
Coverage c:\Build\Main.exe C:\Report
Coverage c:\Build\Main.exe C:\Report -i C:\MainCode C:\Utils\ -x C:\ExternalCode\
)"};

		if (argc - 1 < 2)
		{
			throw std::runtime_error(syntax);
		}

		GLib::Span<char *> const args { argv+1, static_cast<std::ptrdiff_t>(argc)-1 };

		auto it = args.begin();
		auto end = args.end();
		const auto * const executable = *it++;
		const auto * const reportPath = *it++;
		bool debugChildProcesses{};

		Strings includes;
		Strings excludes;
		while (it != end)
		{
			const auto * const arg = *it++;
			if (strcmp(arg, "-?") == 0)
			{
				std::cout << desc << detail;
				return errorCode;
			}
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
			else
			{
				throw std::runtime_error("Unexpected: "s + arg);
			}
		}

		const GLib::Flog::ScopeLog & scope = {GLib::Flog::LogManager::GetLog("Main"), GLib::Flog::Level::Info, "Total"};
		(void)scope;

		Coverage dbg(executable, debugChildProcesses, includes, excludes);
		constexpr unsigned TimeoutMilliseconds = 1000; // just use INFINITE?
		while(dbg.ProcessEvents(TimeoutMilliseconds))
		{}

		HtmlReport report(executable, reportPath, dbg.GetCoverageData());
		(void)report;
	}
	catch (const std::exception & e)
	{
		GLib::Flog::LogManager::GetLog("Main").Error(e.what());
		std::cout << "\x1b[31m" << "\x1b[1m" << e.what() << "\x1b[m" << '\n';

		errorCode = 1;
	}
	return errorCode;
}
