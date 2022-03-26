#pragma once

#include <GLib/Xml/AttributeIterator.h>
#include <GLib/Xml/Iterator.h>

namespace GLib::Xml
{
	inline bool operator==(const Attribute & a1, const Attribute & a2)
	{
		return a1.Name == a2.Name && a1.Value == a2.Value && a1.NameSpace == a2.NameSpace;
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
		return s << "Attr: [" << a.NameSpace << "]:[" << a.Name << "]=[" << a.Value << ']' << std::endl;
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
			if ((*it1).Name != (*it2).Name || (*it1).NameSpace != (*it2).NameSpace || (*it1).Value != (*it2).Value)
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

		if (!e.Attributes().Empty())
		{
			s << std::endl;
			for (const auto & a : e.Attributes())
			{
				s << "Attr: [" << a.NameSpace << "]:[" << a.Name << "]=[" << a.Value << ']' << std::endl;
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
				static_cast<void>(a);
			}
		}
	}
}
