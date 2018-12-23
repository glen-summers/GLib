#pragma once

#include <GLib/Win/Debugger.h>

#include <map>
#include <set>
#include <iterator>
#include <regex>

namespace GLib
{
	class Coverage : public Win::Debugger
	{
		static constexpr unsigned char debugBreakByte = 0xCC;
		std::regex const namespaceRegex{ R"(^(?:[A-Za-z_][A-Za-z_0-9]*::)*)" }; // +some extra unicode chars?

		typedef std::set<unsigned int> Lines;
		typedef std::map<std::wstring, Lines> FileLines;
		class Address;
		typedef std::map<uint64_t, Address> Addresses;
		typedef std::set<std::wstring> WideStrings;
		typedef std::set<std::string> Strings;

		struct LinesCoverage
		{
			size_t linesCovered{}, linesNotCovered{};
		};

		struct Function
		{
			size_t id;
			std::string name, typeName, nameSpace;
			LinesCoverage coverage;
			FileLines fileLines;
		};

		class Address
		{
			unsigned char oldData;
			FileLines fileLines; // use sparse container
			bool visited;

		public:
			Address(unsigned char oldData) : oldData(oldData), visited()
			{}

			unsigned char OldData() const { return oldData; }
			const FileLines & FileLines() const { return fileLines; }
			bool Visited() const { return visited; }

			void Visit()
			{
				visited = true;
			}

			void AddFileLine(const std::wstring & fileName, unsigned int lineNumber)
			{
				fileLines[fileName].insert(lineNumber);
			}
		};

		std::string executable;
		std::string reportPath;
		Addresses addresses;
		std::map<unsigned int, HANDLE> threads;
		WideStrings excludeFilter; // make case insensitive
		std::map<std::wstring, size_t> files;

	public:
		Coverage(const std::string & executable, const std::string & reportPath, const std::set<std::string> & excludes)
			: Debugger(executable)
			, executable(executable)
			, reportPath(reportPath)
			, excludeFilter(a2w(excludes))
		{}

		std::string CreateReport(unsigned int processId);

	private:
		static WideStrings a2w(const Strings & strings)
		{
			WideStrings wideStrings;
			std::transform(strings.begin(), strings.end(), std::inserter(wideStrings, wideStrings.begin()), Cvt::a2w);
			return wideStrings;
		}

		void OnCreateProcess(DWORD processId, DWORD threadId, const CREATE_PROCESS_DEBUG_INFO & info) override;

		void OnExitProcess(DWORD processId, DWORD threadId, const EXIT_PROCESS_DEBUG_INFO& info) override;

		void OnCreateThread(DWORD processId, DWORD threadId, const CREATE_THREAD_DEBUG_INFO & info) override;

		void OnExitThread(DWORD processId, DWORD threadId, const EXIT_THREAD_DEBUG_INFO & info) override;

		DWORD OnException(DWORD processId, DWORD threadId, const EXCEPTION_DEBUG_INFO & info) override;
	};
}
