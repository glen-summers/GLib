#ifndef SCOPE_H
#define SCOPE_H

#include <utility>

namespace GLib::Detail
{
	template <typename Function>
	class ScopedFunction
	{
		Function function;

	public:
		explicit ScopedFunction(Function function)
			: function(function)
		{}

		ScopedFunction(const ScopedFunction &) = delete;
		ScopedFunction & operator=(const ScopedFunction &) = delete;
		ScopedFunction(ScopedFunction &&) noexcept = default;
		ScopedFunction & operator=(ScopedFunction &&) noexcept = default;

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

#define SCOPE_IMPL(name, line, func) /*NOLINT*/                                                                                                      \
	const auto & name##line = GLib::Detail::Scope(func);                                                                                               \
	(void) name##line;
#define SCOPE_JOIN(name, line, func) SCOPE_IMPL(name, line, func) /*NOLINT*/
#define SCOPE(name, func) SCOPE_JOIN(name, __LINE__, func) /*NOLINT*/

#endif // SCOPE_H
