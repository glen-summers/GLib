#include "pch.h"

#include "Process.h"

// ren ProcessInfo?
Process::Process(ULONG const processId)
	: processId {processId}
{}

ULONG Process::Id() const
{
	return processId;
}

Addresses const & Process::Addresses() const
{
	return addresses;
}

Threads const & Process::Threads() const
{
	return threads;
}

void Process::AddThread(ULONG const threadId, GLib::Win::HandleBase * const handle)
{
	threads.emplace(threadId, handle);
}

void Process::RemoveThread(ULONG const threadId)
{
	threads.erase(threadId);
}

HANDLE Process::FindThread(ULONG const threadId) const
{
	auto const iter = threads.find(threadId);
	if (iter == threads.end())
	{
		throw std::runtime_error("Thread not found");
	}
	return iter->second;
}

IndexToFunction const & Process::IndexToFunction() const
{
	return indexToFunction;
}

Addresses::const_iterator Process::AddAddress(uint64_t const address, Address address1)
{
	return addresses.emplace(address, std::move(address1)).first;
}

IndexToFunction::const_iterator Process::AddFunction(ULONG const index, std::string const & nameSpace, std::string const & typeName,
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