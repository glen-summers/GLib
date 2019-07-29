#ifndef SCOPE_H
#define SCOPE_H

#include <utility>

namespace GLib
{
	namespace Detail
	{
		// use a UniquePtr?
		template <typename Function>
		class ScopedFunction
		{
			Function function;

		public:
			ScopedFunction(Function function) : function(function)
			{}

			ScopedFunction(const ScopedFunction &) = delete;
			ScopedFunction & operator=(const ScopedFunction &) = delete;
			ScopedFunction(ScopedFunction &&) = default;
			ScopedFunction & operator=(ScopedFunction &&) = default;

			~ScopedFunction()
			{
				function();
			}
		};

		template <typename Function>
		auto Scope(Function && exit)
		{
			return Detail::ScopedFunction<Function>(std::forward<Function>(exit));
		}
	}
}


#define SCOPE_IMPL(name, line, func) const auto & name##line = GLib::Detail::Scope(func); (void)name##line;
#define SCOPE_JOIN(name, line, func) SCOPE_IMPL(name, line, func)
#define SCOPE(name, func) SCOPE_JOIN(name, __LINE__, func)

#endif // SCOPE_H
