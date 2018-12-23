#include "pch.h"

#include "Coverage.h"

#include <iostream>

int main(int argc, char *argv[])
{
	using namespace std::string_literals;

	try
	{
		if (argc < 3)
		{
			throw std::runtime_error("Coverage <Executable> <Report> [-i IncludePath]... [-x excludePath]...");
		}

		const auto executable = argv[1];
		const auto report = argv[2];

		GLib::Strings includes, excludes;
		for (int i = 3; i < argc; ++i)
		{
			if (strcmp(argv[i], "-i") == 0)
			{
				if(++i== argc)
				{
					throw std::runtime_error("Missing include value");
				}
				includes.insert(argv[i]);
			}
			else if (strcmp(argv[i], "-x") == 0)
			{
				if(++i== argc)
				{
					throw std::runtime_error("Missing include value");
				}
				excludes.insert(argv[i]);
			}
			else
			{
				throw std::runtime_error("Unexpected: "s + argv[i]);
			}
		}

		GLib::Coverage dbg(executable, report, includes, excludes);
		for(;dbg.ProcessEvents(1000);) // just use INFINITE?
		{}
		std::cout << "Exited: Process exited with code: " << dbg.ExitCode() << std::endl;
	}
	catch (const std::exception & e)
	{
 		std::cout << e.what() << std::endl;
	}
}
