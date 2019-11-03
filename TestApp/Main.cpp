
#include <GLib/Win/Resources.h>
#include <GLib/Win/Window.h>

#include <GLib/flogging.h>
#include <GLib/split.h>

using namespace std::chrono_literals;

int IDI_ICON = 0;
int IDC_MAIN = 0;
int IDS_APP_TITLE = 1;

class MainWindow : public GLib::Win::Window
{
	inline static GLib::Flog::Log log = GLib::Flog::LogManager::GetLog<MainWindow>();

	unsigned int const exitTimeSeconds;

public:
	MainWindow(HINSTANCE /*instance*/, const GLib::Win::Size & /*size*/, unsigned int exitTimeSeconds)
		: GLib::Win::Window(IDI_ICON, IDC_MAIN, "TestApp")
		, exitTimeSeconds(exitTimeSeconds)
	{
		log.Info("Ctor");
		SetTimer(std::chrono::seconds{exitTimeSeconds});
	}

protected:
	void OnDestroy() noexcept override
	{
		log.Info("OnDestroy");
		::PostQuitMessage(0);
	}

	bool OnTimer() noexcept override
	{
		log.Info("Tick");
		Destroy();
		return true;
	}
};

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prevInstance, _In_ LPWSTR cmdLine, _In_ int cmdShow)
{
	(void)prevInstance;

	int returnValue{};
	GLib::Flog::Log log = GLib::Flog::LogManager::GetLog("Main");

	try
	{
		auto cmd = GLib::Cvt::w2a(cmdLine);
		log.Info("Cmd: [{0}]",  cmd);

		GLib::Util::Splitter split{cmd, " "};
		unsigned int exitTime{1};
		for (auto it = split.begin(); it!=split.end(); ++it)
		{
			if (*it=="-exitTime" && ++it != split.end())
			{
				if (!(std::istringstream{*it} >> exitTime))
				{
					throw std::runtime_error("Parse error");
				}
			}
		}

		log.Info("Create");
		MainWindow window(instance, { 1024, 768 }, exitTime);

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