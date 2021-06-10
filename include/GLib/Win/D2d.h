#pragma once

#include <GLib/Win/ComPtr.h>

#include <d2d1.h>
#include <d2d1_3.h>
#pragma comment(lib, "D2d1.lib")

namespace GLib::Win::D2d
{
	namespace Detail
	{
		inline ComPtr<ID2D1Factory> CreateFactory()
		{
			GLib::Win::ComPtr<ID2D1Factory> factory;
			D2D1_FACTORY_OPTIONS options {D2D1_DEBUG_LEVEL_INFORMATION};
			GLib::Win::CheckHr(
				::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), &options, GLib::Win::GetAddress<ID2D1Factory>(factory)),
				"D2D1CreateFactory");
			return factory;
		}
	}

	class Factory
	{
		ComPtr<ID2D1Factory> factory {Detail::CreateFactory()};

	public:
		ComPtr<ID2D1HwndRenderTarget> CreateRenderTarget(HWND handle, const SIZE & size) const
		{
			ComPtr<ID2D1HwndRenderTarget> renderTarget;

			GLib::Win::WarnHr(factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
																												D2D1::HwndRenderTargetProperties(handle, D2D1::SizeU(size.cx, size.cy)),
																												GetAddress(renderTarget)),
												"CreateHwndRenderTarget");
			return renderTarget;
		}
	};

	class Renderer
	{
		Factory factory;
		ComPtr<ID2D1HwndRenderTarget> renderTarget;
		bool recreated;

	public:
		Renderer(Factory factory)
			: factory(std::move(factory))
			, recreated()
		{}

		const Factory & Factory() const
		{
			return factory;
		}

		void Verify(HWND handle, const SIZE & size)
		{
			bool recreate = !renderTarget;
			if (!renderTarget)
			{
				renderTarget = factory.CreateRenderTarget(handle, size);
			}
			recreated = recreate; // how reset
		}

		template <typename Function>
		void CheckDevice(const Function & function) const
		{
			if (recreated)
			{
				function(*this);
			}
		}

		void Begin() const // take a Labda and encompass Recreate here
		{
			renderTarget->BeginDraw();
		}

		void End()
		{
			HRESULT hr = renderTarget->EndDraw();
			if (hr == D2DERR_RECREATE_TARGET)
			{
				renderTarget = nullptr;
			}
			else
			{
				WarnHr(hr, "EndDraw");
			}
		}

		bool Resize(const SIZE & size) const
		{
			bool resize = renderTarget != nullptr;
			if (resize)
			{
				renderTarget->Resize(D2D1::SizeU(size.cx, size.cy));
			}
			return resize;
		}

		void Clear(const D2D1::ColorF & colour) const
		{
			renderTarget->Clear(colour);
		}
	};
}