#ifndef SCOPE_H
#define SCOPE_H

#include <utility>

namespace GLib
{
	namespace Detail
	{
		// use a UniquePtr?
		template <typename T>
		struct ScopeImpl
		{
			ScopeImpl(const ScopeImpl &) = delete;
			ScopeImpl & operator=(const ScopeImpl &) = delete;
			ScopeImpl(ScopeImpl &&) = default;
			ScopeImpl & operator=(ScopeImpl &&) = default;

			ScopeImpl(T t) : t(t) {}
			T t;
			~ScopeImpl() {
				t();
			}
		};


		template <typename T>
		auto Scope(T && exit)
		{
			return Detail::ScopeImpl<T>(std::forward<T>(exit));
		}
	}
}


#define SCOPE_IMPL(name, line, func) const auto & name##line = GLib::Detail::Scope(func); (void)name##line;
#define SCOPE_JOIN(name, line, func) SCOPE_IMPL(name, line, func)
#define SCOPE(name, func) SCOPE_JOIN(name, __LINE__, func)

#endif // SCOPE_H
