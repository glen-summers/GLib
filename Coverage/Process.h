#pragma once

#include "Function.h"

#include <GLib/Formatter.h>

#include <unordered_map>

class Process;
using Threads = std::unordered_map<ULONG, void *>;
using IndexToFunction = std::unordered_map<ULONG, Function>;
using Processes = std::unordered_map<ULONG, Process>;

class Process // ren ProcessInfo
{
	ULONG const processId;
	Addresses addresses;
	Threads threads;
	IndexToFunction indexToFunction;

public:
	explicit Process(ULONG processId);

	[[nodiscard]] ULONG Id() const;

	[[nodiscard]] Addresses const & Addresses() const;

	[[nodiscard]] Threads const & Threads() const;

	void AddThread(ULONG threadId, GLib::Win::HandleBase * handle);

	void RemoveThread(ULONG threadId);

	[[nodiscard]] HANDLE FindThread(ULONG threadId) const;

	[[nodiscard]] IndexToFunction const & IndexToFunction() const;

	Addresses::const_iterator AddAddress(uint64_t address, Address address1);

	IndexToFunction::const_iterator AddFunction(ULONG index, std::string const & nameSpace, std::string const & typeName,
																							std::string const & functionName);
};
