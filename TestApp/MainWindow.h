#pragma once

#include <Windows.h>
#include <Commctrl.h>

#include <GLib/Win/D2d.h>
#include <GLib/Win/Painter.h>
#include <GLib/Win/Window.h>

#include <GLib/flogging.h>

#include "resource.h"

using namespace GLib::Win;

class MainWindow : public Window
{
	inline static GLib::Flog::Log log = GLib::Flog::LogManager::GetLog<MainWindow>();

	D2d::Factory factory;
	D2d::Renderer renderer{factory};
	unsigned int const exitTimeSeconds;

public:
	MainWindow(HINSTANCE /*instance*/, const Size & /*size*/, unsigned int exitTimeSeconds)
		: Window(0, IDR_MENU, IDR_ACCELERATOR, "TestApp")
		, exitTimeSeconds(exitTimeSeconds)
	{
		log.Info("Ctor");
		SetTimer(std::chrono::seconds{exitTimeSeconds});
	}

protected:
	void OnCommand(int command) noexcept override
	{
		switch (command)
		{
			case ID_HELP_ABOUT:
			{
				int button;
				WarnHr(::TaskDialog(Handle(), Instance(), L"About", L"Main", L"Sub", TDCBF_CLOSE_BUTTON, TD_INFORMATION_ICON, &button), "TaskDialog");
				break;
			}

			case ID_FILE_EXIT:
			{
				Close();
				break;
			}
		}
	}

	void OnPaint() noexcept override
	{
		log.Info("OnPaint");
		Painter p = GetPainter();
		(void)p;

		renderer.Verify(Handle(), ClientSize());
		renderer.Begin();
		renderer.Clear(D2D1::ColorF::CornflowerBlue);
		renderer.End();
	}

	void OnSize(const Size & size) noexcept override
	{
		if (renderer.Resize(size))
		{
			Invalidate(false);
		}
	}

	CloseResult OnClose() noexcept
	{
		log.Info("OnClose");
		return CloseResult::Allow;
	}

	void OnDestroy() noexcept override
	{
		log.Info("OnDestroy");
		::PostQuitMessage(0);
	}

	void OnTimer() noexcept override
	{
		log.Info("Tick");
		Destroy();
	}
};
