#pragma once

#include <GLib/Win/ComErrorCheck.h>

namespace GLib::Win
{
	namespace Detail
	{
		inline VARIANT Create()
		{
			VARIANT v;
			::VariantInit(&v);
			return v;
		}

		// clang-format off
		inline VARTYPE Vt(const VARIANT & v) { return v.vt; } // NOLINT(cppcoreguidelines-pro-type-union-access) legacy code
		inline VARTYPE & Vt(VARIANT & v) { return v.vt; } // NOLINT(cppcoreguidelines-pro-type-union-access) legacy code
		inline BSTR & Bstr(VARIANT & v) { return v.bstrVal; } // NOLINT(cppcoreguidelines-pro-type-union-access) legacy code
		// clang-format on

		inline VARIANT Create(const std::string & value)
		{
			auto * bstr = ::SysAllocString(Cvt::a2w(value).c_str());
			if (bstr == nullptr)
			{
				throw std::runtime_error("SysAllocString");
			}

			VARIANT v;
			::VariantInit(&v);
			Vt(v) = VT_BSTR;
			Bstr(v) = ::SysAllocString(Cvt::a2w(value).c_str());
			return v;
		}

		inline VARIANT Copy(const VARIANT & v)
		{
			VARIANT copy;
			::VariantInit(&copy);
			GLib::Win::CheckHr(::VariantCopy(&copy, &v), "VariantCopy");
			return copy;
		}

		inline VARIANT Move(VARIANT & other)
		{
			VARIANT v {other};
			::VariantInit(&other);
			return v;
		}
	}

	// get\set a C++ variant?
	class Variant
	{
		VARIANT v {};

	public:
		Variant()
			: v {Detail::Create()}
		{}

		Variant(const std::string & value)
			: v {Detail::Create(value)}
		{}

		Variant(const Variant & other)
			: v {Detail::Copy(other.v)}
		{}

		Variant(Variant && other) noexcept
			: v {Detail::Move(other.v)}
		{}

		~Variant()
		{
			GLib::Win::WarnHr(::VariantClear(&v), "VariantClear");
		}

		VARTYPE Type() const
		{
			return Detail::Vt(v);
		}

		std::string ToString() const
		{
			VARIANT tmp {};
			GLib::Win::CheckHr(::VariantChangeType(&tmp, &v, 0, VT_BSTR), "VariantChangeType");
			return Cvt::w2a(Detail::Bstr(tmp));
		}

		// swapicle
		Variant & operator=(const Variant & other)
		{
			GLib::Win::CheckHr(::VariantCopy(&v, &other.v), "VariantCopy");
			return *this;
		}

		Variant & operator=(Variant && other) noexcept
		{
			v = other.v;
			::VariantInit(&other.v);
			return *this;
		}

		bool operator==(const Variant & other) const noexcept
		{
			auto * v1 = const_cast<VARIANT *>(&v);			 // NOLINT(cppcoreguidelines-pro-type-const-cast) no const VarCmp
			auto * v2 = const_cast<VARIANT *>(&other.v); // NOLINT(cppcoreguidelines-pro-type-const-cast) no const VarCmp
			return ::VarCmp(v1, v2, 0) == VARCMP_EQ;
		}

		bool operator!=(const Variant & other) const noexcept
		{
			return !(*this == other);
		}
	};
}