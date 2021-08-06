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

	inline std::ostream & operator<<(std::ostream & s, State state)
	{
		return s << static_cast<int>(state);
	}

	inline std::ostream & operator<<(std::ostream & s, const Attribute & a)
	{
		return s << "Attr: [" << a.nameSpace << "]:[" << a.name << "]=[" << a.value << ']' << std::endl;
	}

	inline bool operator==(const Element & e1, const Element & e2)
	{
		if (e1.Type() != e2.Type() || e1.QName() != e2.QName() || e1.NameSpace() != e2.NameSpace() || e1.Text() != e2.Text())
		{
			return false;
		}

		for (auto it1 = e1.Attributes().begin(), it2 = e2.Attributes().begin(); it1 != e1.Attributes().begin() && it2 != e2.Attributes().begin();
				 ++it1, ++it2)
		{
			if (it1 == e1.Attributes().end() || it2 == e2.Attributes().end())
			{
				return false;
			}
			if ((*it1).name != (*it2).name || (*it1).nameSpace != (*it2).nameSpace || (*it1).value != (*it2).value)
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
		s << "NameSpace: [" << e.NameSpace() << "], Name: [" << e.Name() << "], type : [" << static_cast<unsigned int>(e.Type()) << "], text : ["
			<< e.Text() << ']';

		if (!e.Attributes().empty())
		{
			s << std::endl;
			for (const auto & a : e.Attributes())
			{
				s << "Attr: [" << a.nameSpace << "]:[" << a.name << "]=[" << a.value << ']' << std::endl;
			}
		}
		return s;
	}

	inline void Parse(std::string_view xml) // +expectexception
	{
		for (const auto & e : Holder {xml})
		{
			for (const auto & a : e.Attributes())
			{
				(void) a;
			}
		}
	}
}
