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

		inline VARTYPE Vt(VARIANT const & v)
		{
			return v.vt; // NOLINT(cppcoreguidelines-pro-type-union-access)
		}

		inline VARTYPE & Vt(VARIANT & v)
		{
			return v.vt; // NOLINT(cppcoreguidelines-pro-type-union-access)
		}

		inline BSTR & Bstr(VARIANT & v)
		{
			return v.bstrVal; // NOLINT(cppcoreguidelines-pro-type-union-access)
		}

		inline VARIANT Create(std::string const & value)
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

		inline VARIANT Copy(VARIANT const & v)
		{
			VARIANT copy;
			VariantInit(&copy);
			CheckHr(VariantCopy(&copy, &v), "VariantCopy");
			return copy;
		}

		inline VARIANT Move(VARIANT & other)
		{
			VARIANT const v {other};
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

		explicit Variant(std::string const & value)
			: v {Detail::Create(value)}
		{}

		Variant(Variant const & other)
			: v {Detail::Copy(other.v)}
		{}

		Variant(Variant && other) noexcept
			: v {Detail::Move(other.v)}
		{}

		~Variant()
		{
			WarnHr(VariantClear(&v), "VariantClear");
		}

		[[nodiscard]] VARTYPE Type() const
		{
			return Detail::Vt(v);
		}

		[[nodiscard]] std::string ToString() const
		{
			VARIANT tmp {};
			CheckHr(VariantChangeType(&tmp, &v, 0, VT_BSTR), "VariantChangeType");
			return Cvt::W2A(Detail::Bstr(tmp));
		}

		Variant & operator=(Variant const & other)
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

		bool operator==(Variant const & other) const noexcept
		{
			auto * v1 = const_cast<VARIANT *>(&v);			 // NOLINT(cppcoreguidelines-pro-type-const-cast) required
			auto * v2 = const_cast<VARIANT *>(&other.v); // NOLINT(cppcoreguidelines-pro-type-const-cast)
			return VarCmp(v1, v2, 0) == VARCMP_EQ;
		}

		bool operator!=(Variant const & other) const noexcept
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