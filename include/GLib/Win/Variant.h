#pragma once

#include "GLib/Win/ComErrorCheck.h"

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

		inline VARIANT Create(const std::string & value)
		{
			VARIANT v;
			::VariantInit(&v);
			v.vt = VT_BSTR;
			v.bstrVal = ::SysAllocString(Cvt::a2w(value).c_str());
			if (!v.bstrVal)
			{
				throw std::runtime_error("SysAllocString");
			}
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
			VARIANT v{other};
			::VariantInit(&other);
			return v;
		}
	}

	// get\set a C++ variant?
	class Variant
	{
		VARIANT v{};

	public:
		Variant() : v{Detail::Create()}
		{}

		Variant(const std::string & value) : v{Detail::Create(value)}
		{}

		Variant(const Variant & other) : v{Detail::Copy(other.v)}
		{}

		Variant(Variant && other) noexcept : v{Detail::Move(other.v)}
		{}

		~Variant()
		{
			GLib::Win::WarnHr(::VariantClear(&v), "VariantClear");
		}

		VARTYPE Type() const
		{
			return v.vt;
		}

		std::string ToString() const
		{
			VARIANT tmp{};
			GLib::Win::CheckHr(::VariantChangeType(&tmp, &v, 0, VT_BSTR), "VariantChangeType");
			return Cvt::w2a(V_BSTR(&tmp));
		}

		Variant & operator = (const Variant & other)
		{
			GLib::Win::CheckHr(::VariantCopy(&v, &other.v), "VariantCopy");
			return *this;
		}

		Variant & operator = (Variant && other) noexcept
		{
			v = other.v;
			::VariantInit(&other.v);
			return *this;
		}

		bool operator == (const Variant & other) const noexcept
		{
			return ::VarCmp(const_cast<VARIANT *>(&v), const_cast<VARIANT *>(&other.v), 0) == VARCMP_EQ;
		}

		bool operator != (const Variant & other) const noexcept
		{
			return !(*this == other);
		}
	};
}