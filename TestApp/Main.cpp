
#include "MainWindow.h"
#include <GLib/Split.h>
#include <GLib/Win/ComUtils.h>
#pragma comment(lib, "Comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// NOLINTNEXTLINE(readability-non-const-parameter) 'wWinMain': function cannot be overloaded
int APIENTRY wWinMain(_In_ HINSTANCE const hInstance, _In_opt_ HINSTANCE const hPrevInstance, _In_ LPWSTR const lpCmdLine, _In_ int const nShowCmd)
{
	static_cast<void>(hPrevInstance);

	int returnValue {};
	GLib::Flog::Log const & log = GLib::Flog::LogManager::GetLog("Main");

	try
	{
		GLib::Win::Mta const com;
		auto cmd = GLib::Cvt::W2A(lpCmdLine);
		log.Info("Cmd: [{0}]", cmd);

		GLib::Util::Splitter const split {cmd, " "};
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

		int constexpr width = 1024;
		int constexpr height = 768;
		TestApp::MainWindow const window(hInstance, {width, height}, exitTime);

		log.Info("show: {0}", nShowCmd);
		if (nShowCmd != 0)
		{
			static_cast<void>(window.Show(nShowCmd));
		}
		log.Info("Pump");
		returnValue = window.PumpMessages();
		static_cast<void>(com);
	}
	catch (std::runtime_error const & e)
	{
		log.Info("Exception: {0}", e.what());
		returnValue = 1;
	}
	log.Info("Exit");
	return returnValue;
}
