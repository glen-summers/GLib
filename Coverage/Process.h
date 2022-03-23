#pragma once

#include "Function.h"

#include <GLib/Formatter.h>

#include <unordered_map>

class Process;
using Threads = std::unordered_map<unsigned int, void *>;
using IndexToFunction = std::unordered_map<unsigned long, Function>;
using Processes = std::unordered_map<unsigned int, Process>;

class Process // ren ProcessInfo
{
	unsigned int id;
	Addresses addresses;
	Threads threads;
	IndexToFunction indexToFunction;

public:
	Process(unsigned int id)
		: id {id}
	{}

	const Addresses & Addresses() const
	{
		return addresses;
	}

	const Threads & Threads() const
	{
		return threads;
	};

	void AddThread(DWORD threadId, HANDLE handle)
	{
		threads.emplace(threadId, handle);
	}

	void RemoveThread(DWORD threadId)
	{
		threads.erase(threadId);
	}

	HANDLE FindThread(DWORD threadId) const
	{
		auto it = threads.find(threadId);
		if (it == threads.end())
		{
			throw std::runtime_error("Thread not found");
		}
		return it->second;
	}

	const IndexToFunction & IndexToFunction() const
	{
		return indexToFunction;
	}

	auto AddAddress(DWORD64 address, Address address1)
	{
		return addresses.emplace(address, std::move(address1)).first;
	}

	auto AddFunction(ULONG index, const std::string & nameSpace, const std::string & typeName, const std::string & functionName)
	{
		// double lookup to allow moving strings?
		auto ret = indexToFunction.emplace(index, Function {nameSpace, typeName, functionName});
		if (!ret.second)
		{
			throw std::runtime_error(GLib::Formatter::Format("Duplicate function id:{0}, {1}:{2}:{3}", index, nameSpace, typeName, functionName));
		}
		return ret.first;
	}
};
