#pragma once

#include <GLib/Win/ComPtr.h>
#include <GLib/Win/Handle.h>

#include <d2d1.h>
#pragma comment(lib, "D2d1.lib")

namespace GLib::Win::D2d
{
	namespace Detail
	{
		inline ComPtr<ID2D1Factory> CreateFactory()
		{
			ComPtr<ID2D1Factory> factory;
			D2D1_FACTORY_OPTIONS constexpr options {D2D1_DEBUG_LEVEL_INFORMATION};
			CheckHr(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, GetUuId<ID2D1Factory>(), &options, GetAddress(factory).Void()),
							"D2D1CreateFactory");
			return factory;
		}
	}

	class Factory
	{
		ComPtr<ID2D1Factory> factory {Detail::CreateFactory()};

	public:
		ComPtr<ID2D1HwndRenderTarget> CreateRenderTarget(WindowHandleBase * const handle, SIZE const & size) const
		{
			ComPtr<ID2D1HwndRenderTarget> renderTarget;
			WarnHr(factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(handle, D2D1::SizeU(size.cx, size.cy)),
																						 GetAddress(renderTarget).Raw()),
						 "CreateHwndRenderTarget");
			return renderTarget;
		}
	};

	class Renderer
	{
		Factory factory;
		ComPtr<ID2D1HwndRenderTarget> renderTarget;
		bool recreated {};

	public:
		explicit Renderer(Factory factory)
			: factory(std::move(factory))
		{}

		[[nodiscard]] Factory const & Factory() const
		{
			return factory;
		}

		void Verify(WindowHandleBase * const handle, SIZE const & size)
		{
			bool const recreate = !renderTarget;
			if (!renderTarget)
			{
				renderTarget = factory.CreateRenderTarget(handle, size);
			}
			recreated = recreate; // how reset
		}

		template <typename Function>
		void CheckDevice(Function const & function) const
		{
			if (recreated)
			{
				function(*this);
			}
		}

		void Begin() const // take a lambda and encompass Recreate here
		{
			renderTarget->BeginDraw();
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

		[[nodiscard]] bool Resize(SIZE const & size) const
		{
			bool const resize = renderTarget != nullptr;
			if (resize)
			{
				renderTarget->Resize(D2D1::SizeU(size.cx, size.cy));
			}
			return resize;
		}

		void Clear(D2D1 ::ColorF const & colour) const
		{
			renderTarget->Clear(colour);
		}
	};
}