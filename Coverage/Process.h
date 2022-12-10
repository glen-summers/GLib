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
	explicit Process(ULONG const processId)
		: processId {processId}
	{}

	[[nodiscard]] ULONG Id() const
	{
		return processId;
	}

	[[nodiscard]] Addresses const & Addresses() const
	{
		return addresses;
	}

	[[nodiscard]] Threads const & Threads() const
	{
		return threads;
	}

	void AddThread(ULONG const threadId, GLib::Win::HandleBase * const handle)
	{
		threads.emplace(threadId, handle);
	}

	void RemoveThread(ULONG const threadId)
	{
		threads.erase(threadId);
	}

	[[nodiscard]] HANDLE FindThread(ULONG const threadId) const
	{
		auto const iter = threads.find(threadId);
		if (iter == threads.end())
		{
			throw std::runtime_error("Thread not found");
		}
		return iter->second;
	}

	[[nodiscard]] IndexToFunction const & IndexToFunction() const
	{
		return indexToFunction;
	}

	Addresses::const_iterator AddAddress(uint64_t const address, Address address1)
	{
		return addresses.emplace(address, address1).first;
	}

	IndexToFunction::const_iterator AddFunction(ULONG const index, std::string const & nameSpace, std::string const & typeName,
																							std::string const & functionName)
	{
		// double lookup to allow moving strings?
		auto const [it, inserted] = indexToFunction.emplace(index, Function {nameSpace, typeName, functionName});
		if (!inserted)
		{
			throw std::runtime_error(GLib::Formatter::Format("Duplicate function id:{0}, {1}:{2}:{3}", index, nameSpace, typeName, functionName));
		}
		return it;
	}
};
