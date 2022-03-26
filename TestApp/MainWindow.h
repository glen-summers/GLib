#pragma once

#include <Windows.h>

#include <CommCtrl.h>

#include <GLib/Win/D2d.h>
#include <GLib/Win/DebugWrite.h>
#include <GLib/Win/Painter.h>
#include <GLib/Win/Window.h>

#include <GLib/Flogging.h>

#include "resource.h"

namespace TestApp
{
	class MainWindow : public GLib::Win::Window
	{
		inline static const GLib::Flog::Log log = GLib::Flog::LogManager::GetLog<MainWindow>();

		GLib::Win::D2d::Factory factory;
		GLib::Win::D2d::Renderer renderer {factory};

	public:
		MainWindow(HINSTANCE /*instance*/, const GLib::Win::Size & /*size*/, unsigned int exitTimeSeconds)
			: Window(0, IDR_MENU, IDR_ACCELERATOR, "TestApp")
		{
			log.Info("Ctor");
			SetTimer(std::chrono::seconds {exitTimeSeconds});
		}

		MainWindow(const MainWindow &) = delete;
		MainWindow(MainWindow &&) = delete;
		MainWindow & operator=(const MainWindow &) = delete;
		MainWindow & operator=(MainWindow &&) = delete;
		~MainWindow() override = default;

	protected:
		void OnCommand(int command) noexcept override
		{
			switch (command)
			{
				case ID_HELP_ABOUT:
				{
					int button {};
					GLib::Win::WarnHr(TaskDialog(Handle(), Instance(), L"About", L"Main", L"Sub", TDCBF_CLOSE_BUTTON, TD_INFORMATION_ICON, &button),
														"TaskDialog");
					break;
				}

				case ID_FILE_EXIT:
				{
					Close();
					break;
				}

				default:
				{
					break;
				}
			}
		}

		void OnPaint() noexcept override
		{
			log.Info("OnPaint");
			GLib::Win::Painter p = GetPainter();
			renderer.Verify(Handle(), ClientSize());
			renderer.Begin();
			renderer.Clear(D2D1::ColorF::CornflowerBlue);
			renderer.End();
			static_cast<void>(p);
		}

		void OnSize(const GLib::Win::Size & size) noexcept override
		{
			try
			{
				if (renderer.Resize(size))
				{
					Invalidate(false);
				}
			}
			catch (const std::exception & e)
			{
				GLib::Win::Debug::Write("Exception: {0}", e.what());
			}
		}

		GLib::Win::CloseResult OnClose() noexcept override
		{
			log.Info("OnClose");
			return GLib::Win::CloseResult::Allow;
		}

		void OnDestroy() noexcept override
		{
			log.Info("OnDestroy");
			PostQuitMessage(0);
		}

		void OnTimer() noexcept override
		{
			log.Info("Tick");
			Destroy();
		}
	};
}