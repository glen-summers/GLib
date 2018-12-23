#include "pch.h"

#include "Coverage.h"

#include <iostream>

int main(int argc, char *argv[])
{
	try
	{
		if (argc < 3)
		{
			throw std::runtime_error("Coverage <Executable> <Report> [ExcludePaths, ...]");
		}

		GLib::Coverage dbg(argv[1], argv[2], {argv + 3, argv + argc});
		for(;dbg.ProcessEvents(1000);) // just use INFINITE?
		{}
		std::cout << "Exited: Process exited with code: " << dbg.ExitCode() << std::endl;
	}
	catch (const std::exception & e)
	{
 		std::cout << e.what() << std::endl;
	}
}
