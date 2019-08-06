#include "pch.h"

#include "Coverage.h"

#include "GLib/Span.h"

#include <iostream>

using namespace std::string_literals;

int main(int argc, char *argv[])
{
	int errorCode = 0;

	GLib::Span<char *> const args { argv+1, argc-1 };

	try
	{
		if (argc < 3)
		{
			throw std::runtime_error("Coverage <Executable> <Report> [-i IncludePath]... [-x excludePath]...");
		}

		auto it = args.begin();
		auto end = args.end();
		const auto executable = *it++;
		const auto report = *it++;

		Strings includes;
		Strings excludes;
		for (; it!=end; ++it)
		{
			auto arg = *it++;
			if (strcmp(arg, "-i") == 0)
			{
				if (it == end)
				{
					throw std::runtime_error("Missing include value");
				}
				includes.insert(*it);
			}
			else if (strcmp(arg, "-x") == 0)
			{
				if (it == end)
				{
					throw std::runtime_error("Missing exclude value");
				}
				excludes.insert(*it);
			}
			else
			{
				throw std::runtime_error("Unexpected: "s + *it); // NOLINT(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers) bug in clang-tidy
			}
		}

		Coverage dbg(executable, report, includes, excludes);
		constexpr unsigned TimeoutMilliseconds = 1000; // just use INFINITE?
		for(;dbg.ProcessEvents(TimeoutMilliseconds);)
		{}
		std::cout << "Exited: Process exited with code: " << dbg.ExitCode() << std::endl;

	}
	catch (const std::exception & e)
	{
 		std::cout << e.what() << std::endl;
		errorCode = 1;
	}
	return errorCode;
}
