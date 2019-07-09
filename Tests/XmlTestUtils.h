#pragma once

#include <GLib/Xml/Iterator.h>

namespace GLib::Xml
{
	inline bool operator==(const Element & e1, const Element & e2)
	{
		if (e1.type!= e2.type ||
			e1.qName != e2.qName ||
			e1.nameSpace != e2.nameSpace ||
			e1.attributes.size() != e2.attributes.size())
		{
			return false;
		}

		for (size_t i = 0; i < e1.attributes.size(); ++i)
		{
			if (e1.attributes[i].name != e2.attributes[i].name ||
				e1.attributes[i].nameSpace != e2.attributes[i].nameSpace ||
				e1.attributes[i].value != e2.attributes[i].value)
			{
				return false;
			}
		}

		return true;
	}

	inline bool operator!=(const Element & e1, const Element & e2)
	{
		return !(e1 == e2);
	}

	inline std::ostream & operator<<(std::ostream & s, const Element & e)
	{
		s << "NameSpace: " << e.nameSpace << "Name: " << e.name << ", type : " << (int)e.type;
		if (!e.attributes.empty())
		{
			s << std::endl;
			for(const auto & a : e.attributes)
			{
				s << "Attr: " << a.nameSpace << ":" << a.name << "=" << a.value << std::endl;
			}
		}
		return s;
	}

	inline void Parse(const std::string_view & xml) // +expectexception
	{
		for (auto e : Holder{xml})
		{
			(void)e;
		}
	}
}
