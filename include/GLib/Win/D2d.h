#pragma once

#include <GLib/Win/ComPtr.h>
#include <GLib/Win/Handle.h>

#include <d2d1.h>
#pragma comment(lib, "D2d1.lib")

// TODO TextFormat
// #pragma comment(lib, "dwrite.lib")
// #include <dwrite_3.h>

namespace GLib::Win::D2d
{
	inline ComPtr<ID2D1Factory> CreateFactory()
	{
		ComPtr<ID2D1Factory> factory;
		D2D1_FACTORY_OPTIONS constexpr options {D2D1_DEBUG_LEVEL_INFORMATION};
		CheckHr(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, GetUuId<ID2D1Factory>(), &options, GetAddress(factory).Void()), "D2D1CreateFactory");
		return factory;
	}

	class Renderer
	{
		WindowHandleBase * const handle;
		SIZE const size;

		ComPtr<ID2D1Factory> factory {CreateFactory()};
		ComPtr<ID2D1HwndRenderTarget> renderTarget {CreateRenderTarget()};
		bool recreated {};

	public:
		Renderer(WindowHandleBase * const handle, SIZE const & size)
			: handle {handle}
			, size {size}
		{}

		Renderer(Renderer &) = delete;
		Renderer(Renderer &&) = delete;
		Renderer & operator=(Renderer const &) = delete;
		Renderer & operator=(Renderer &&) = delete;
		virtual ~Renderer() = default;

		ComPtr<ID2D1HwndRenderTarget> CreateRenderTarget()
		{
			ComPtr<ID2D1HwndRenderTarget> target;
			CheckHr(factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(handle, D2D1::SizeU(size.cx, size.cy)),
																							GetAddress(target).Raw()),
							"CreateHwndRenderTarget");
			return target;
		}

		[[nodiscard]] ComPtr<ID2D1Brush> CreateBrush(D2D1::ColorF const colour) const
		{
			ComPtr<ID2D1SolidColorBrush> brush;
			CheckHr(renderTarget->CreateSolidColorBrush(colour, GetAddress(brush).Raw()), "CreateSolidColorBrush");
			return ComCast<ID2D1Brush>(brush);
		}

		[[nodiscard]] ComPtr<ID2D1Factory> const & Factory() const
		{
			return factory;
		}

		void Verify()
		{
			bool const recreate = !renderTarget;
			if (recreate)
			{
				renderTarget = CreateRenderTarget();
			}
			recreated = recreate; // how reset
		}

		/*template <typename Function>
		void CheckDevice(Function const & function) const
		{
			if (recreated)
			{
				function(*this);
			}
		}*/

		void Begin() const // take a lambda and encompass Recreate here
		{
			renderTarget->BeginDraw();
		}

		void DrawRect(D2D1_RECT_F const & rect, ComPtr<ID2D1Brush> & brush, FLOAT strokeWidth = 1.0F)
		{
			renderTarget->DrawRectangle(rect, Get(brush), strokeWidth);
		}

		void DrawEllipse(D2D1_ELLIPSE const & ellipse, ComPtr<ID2D1Brush> & brush, FLOAT strokeWidth = 1.0F)
		{
			renderTarget->DrawEllipse(ellipse, Get(brush), strokeWidth);
		}

		void End()
		{
			HRESULT const result = renderTarget->EndDraw();
			if (result == D2DERR_RECREATE_TARGET)
			{
				renderTarget = nullptr;
			}
			else
			{
				WarnHr(result, "EndDraw");
			}
		}

		[[nodiscard]] bool Resize() const
		{
			bool const resize = renderTarget != nullptr;
			if (resize)
			{
				renderTarget->Resize(D2D1::SizeU(size.cx, size.cy));
			}
			return resize;
		}

		void Clear(D2D1::ColorF const & colour) const
		{
			renderTarget->Clear(colour);
		}
	};
}