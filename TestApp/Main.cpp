
#include "MainWindow.h"
#include <GLib/Split.h>
#include <GLib/Win/ComUtils.h>
#pragma comment(lib, "Comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// NOLINTNEXTLINE(readability-non-const-parameter) 'wWinMain': function cannot be overloaded
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
	static_cast<void>(hPrevInstance);

	int returnValue {};
	const GLib::Flog::Log & log = GLib::Flog::LogManager::GetLog("Main");

	try
	{
		GLib::Win::Mta com;
		auto cmd = GLib::Cvt::W2A(lpCmdLine);
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

		const int width = 1024;
		const int height = 768;
		TestApp::MainWindow window(hInstance, {width, height}, exitTime);

		log.Info("show: {0}", nShowCmd);
		if (nShowCmd != 0)
		{
			static_cast<void>(window.Show(nShowCmd));
		}
		log.Info("Pump");
		returnValue = window.PumpMessages();
		static_cast<void>(com);
	}
	catch (const std::runtime_error & e)
	{
		log.Info("Exception: {0}", e.what());
		returnValue = 1;
	}
	log.Info("Exit");
	return returnValue;
}
