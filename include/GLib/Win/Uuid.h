#pragma once

#include <GLib/Formatter.h>
#include <GLib/Win/ComErrorCheck.h>

#include <rpc.h>
#pragma comment(lib, "Rpcrt4.lib")

namespace GLib::Win::Util
{
	class Uuid
	{
		UUID const id;

	public:
		explicit Uuid(UUID const & id)
			: id(id)
		{}

		static Uuid CreateRandom()
		{
			UUID uuid;
			CheckHr(UuidCreate(&uuid), "UuidCreate");
			return Uuid {uuid};
		}

	private:
		std::ostream & WriteTo(std::ostream & s) const
		{
			constexpr auto i5 = 5;
			constexpr auto i6 = 6;
			constexpr auto i7 = 7;
			Formatter::Format(s, "{{{0:%08X}-{1:%04X}-{2:%04X}-{3:%02X}{4:%02X}-{5:%02X}{6:%02X}{7:%02X}{8:%02X}{9:%02X}{10:%02X}}}", id.Data1, id.Data2,
												id.Data3, id.Data4[0], id.Data4[1], id.Data4[2], id.Data4[3], id.Data4[4], id.Data4[i5], id.Data4[i6], id.Data4[i7]);
			return s;
		}

		friend std::ostream & operator<<(std::ostream & s, Uuid const & iid);
	};

	inline std::ostream & operator<<(std::ostream & s, Uuid const & iid)
	{
		return iid.WriteTo(s);
	}

	inline std::string ToString(Uuid const & id)
	{
		std::ostringstream stm;
		stm << id;
		return stm.str();
	}
}