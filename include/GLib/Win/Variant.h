#pragma once

namespace GLib::Win
{
	namespace Detail
	{
		inline VARIANT Create()
		{
			VARIANT v;
			VariantInit(&v);
			return v;
		}

		inline VARTYPE Vt(const VARIANT & v)
		{
			return v.vt;
		}

		inline VARTYPE & Vt(VARIANT & v)
		{
			return v.vt;
		}

		inline BSTR & Bstr(VARIANT & v)
		{
			return v.bstrVal;
		}

		inline VARIANT Create(const std::string & value)
		{
			auto * bstr = SysAllocString(Cvt::A2W(value).c_str());
			if (bstr == nullptr)
			{
				throw std::runtime_error("SysAllocString");
			}

			VARIANT v;
			VariantInit(&v);
			Vt(v) = VT_BSTR;
			Bstr(v) = bstr;
			return v;
		}

		inline VARIANT Copy(const VARIANT & v)
		{
			VARIANT copy;
			VariantInit(&copy);
			CheckHr(VariantCopy(&copy, &v), "VariantCopy");
			return copy;
		}

		inline VARIANT Move(VARIANT & other)
		{
			VARIANT v {other};
			VariantInit(&other);
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

		explicit Variant(const std::string & value)
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
			WarnHr(VariantClear(&v), "VariantClear");
		}

		VARTYPE Type() const
		{
			return Detail::Vt(v);
		}

		std::string ToString() const
		{
			VARIANT tmp {};
			CheckHr(VariantChangeType(&tmp, &v, 0, VT_BSTR), "VariantChangeType");
			return Cvt::W2A(Detail::Bstr(tmp));
		}

		Variant & operator=(const Variant & other)
		{
			Variant {other}.Swap(*this);
			CheckHr(VariantCopy(&v, &other.v), "VariantCopy");
			return *this;
		}

		Variant & operator=(Variant && other) noexcept
		{
			v = other.v;
			VariantInit(&other.v);
			return *this;
		}

		bool operator==(const Variant & other) const noexcept
		{
			auto * v1 = const_cast<VARIANT *>(&v);
			auto * v2 = const_cast<VARIANT *>(&other.v);
			return VarCmp(v1, v2, 0) == VARCMP_EQ;
		}

		bool operator!=(const Variant & other) const noexcept
		{
			return !(*this == other);
		}

	private:
		void Swap(Variant & rhs)
		{
			std::swap(v, rhs.v);
		}
	};
}