#pragma once

#include <GLib/Xml/Iterator.h>
#include <GLib/Xml/AttributeIterator.h>

namespace GLib::Xml
{
	inline bool operator==(const Attribute & a1, const Attribute & a2)
	{
		return a1.name == a2.name && a1.value == a2.value && a1.nameSpace == a2.nameSpace;
	}

	inline bool operator!=(const Attribute & a1, const Attribute & a2)
	{
		return !(a1 == a2);
	}

	inline std::ostream & operator<<(std::ostream & s, const Attribute & a)
	{
		return s << "Attr: " << a.nameSpace << ":" << a.name << "=" << a.value << std::endl;
	}

	inline bool operator==(const Element & e1, const Element & e2)
	{
		if (e1.type!= e2.type ||
			e1.qName != e2.qName ||
			e1.nameSpace != e2.nameSpace)
		{
			return false;
		}

		for (auto it1 = e1.attributes.begin(), it2=e2.attributes.begin();
			it1!=e1.attributes.begin() && it2!=e2.attributes.begin();
			++it1, ++it2)
		{
			if (it1==e1.attributes.end() || it2==e2.attributes.end())
			{
				return false;
			}
			if ((*it1).name != (*it2).name ||
				(*it1).nameSpace != (*it2).nameSpace ||
				(*it1).value != (*it2).value)
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
		s << "NameSpace: " << e.nameSpace << ", Name: " << e.name << ", type : " << (int)e.type;
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
			for (auto a : e.attributes)
			{
				(void)a;
			}
		}
	}
}
