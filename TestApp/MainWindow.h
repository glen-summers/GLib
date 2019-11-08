#pragma once

#include <GLib/Win/D2d.h>
#include <GLib/Win/Painter.h>
#include <GLib/Win/Window.h>

#include <GLib/flogging.h>

constexpr int IDI_ICON = 0;
constexpr int IDC_MAIN = 0;
constexpr int IDS_APP_TITLE = 1;

class MainWindow : public GLib::Win::Window
{
	inline static GLib::Flog::Log log = GLib::Flog::LogManager::GetLog<MainWindow>();

	GLib::Win::D2d::Factory factory;
	GLib::Win::D2d::Renderer renderer{factory};
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
	bool OnPaint() noexcept override
	{
		log.Info("OnPaint");
		GLib::Win::Painter p = GetPainter();
		(void)p;

		renderer.Verify(Handle(), ClientSize());
		renderer.Begin();
		renderer.Clear(D2D1::ColorF::CornflowerBlue);
		renderer.End();

		return true;
	}

	bool OnSize(const GLib::Win::Size & size) noexcept override
	{
		if (renderer.Resize(size))
		{
			Invalidate(false);
		}
		return true;
	}

	bool OnDestroy() noexcept override
	{
		log.Info("OnDestroy");
		::PostQuitMessage(0);
		return true;
	}

	bool OnTimer() noexcept override
	{
		log.Info("Tick");
		Destroy();
		return true;
	}
};
