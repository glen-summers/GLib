
#include "MainWindow.h"

#include <GLib/Win/ComUtils.h>
#include <GLib/split.h>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ LPWSTR cmdLine, _In_ int cmdShow)
{
	(void) prevInstance;

	int returnValue {};
	GLib::Flog::Log log = GLib::Flog::LogManager::GetLog("Main");

	try
	{
		GLib::Win::Mta com;
		auto cmd = GLib::Cvt::w2a(cmdLine);
		log.Info("Cmd: [{0}]", cmd);

		GLib::Util::Splitter split {cmd, " "};
		unsigned int exitTime {1};
		for (auto it = split.begin(); it != split.end(); ++it)
		{
			if (*it == "-exitTime" && ++it != split.end())
			{
				if (!(std::istringstream {*it} >> exitTime))
				{
					throw std::runtime_error("Parse error");
				}
			}
		}

		log.Info("Create");
		MainWindow window(instance, {1024, 768}, exitTime);

		log.Info("show: {0}", cmdShow);
		if (cmdShow != 0)
		{
			window.Show(cmdShow);
		}
		log.Info("Pump");
		returnValue = window.PumpMessages();
	}
	catch (const std::runtime_error & e)
	{
		log.Info("Exception: {0}", e.what());
		returnValue = 1;
	}
	log.Info("Exit");
	return returnValue;
}