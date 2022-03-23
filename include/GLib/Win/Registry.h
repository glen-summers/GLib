#pragma once

#include <GLib/Win/ErrorCheck.h>
#include <GLib/StackOrHeap.h>

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

namespace GLib::Win
{
	using RegistryValue = std::variant<std::string, uint32_t, uint64_t>;

	namespace Detail
	{
		inline static constexpr DWORD Read = KEY_READ;
		inline static constexpr DWORD AllAccess = KEY_ALL_ACCESS;
		inline static constexpr size_t pos = 30;
		inline static constexpr size_t len = 8;

		constexpr ULONG_PTR SignExtend(uint32_t value)
		{
			switch (sizeof(ULONG_PTR))
			{
				case sizeof(uint32_t):
				{
					return static_cast<ULONG_PTR>(value);
				}
				case sizeof(uint64_t):
				{
					constexpr auto magicNumber = 0xffffffff00000000;
					return static_cast<ULONG_PTR>(magicNumber + value);
				}
				default:
				{
					throw std::logic_error("Unexpected");
				}
			}
		}

		constexpr uint32_t Hex(char number)
		{
			constexpr auto ten = 0x0a;
			return (number >= '0' && number <= '9')		? number - '0'
						 : (number >= 'a' && number <= 'f') ? number - 'a' + ten
						 : (number >= 'A' && number <= 'F') ? number - 'A' + ten
																								: throw std::runtime_error("!");
		}

		constexpr uint32_t ToInt32(std::string_view s)
		{
			uint32_t result {};
			for (auto c : s)
			{
				result <<= 4U;
				result += Hex(c);
			}
			return result;
		}

		inline static constexpr ULONG_PTR ClassesRoot = SignExtend(ToInt32(std::string_view {STRINGIFY(HKEY_CLASSES_ROOT)}.substr(pos, len)));
		inline static constexpr ULONG_PTR CurrentUser = SignExtend(ToInt32(std::string_view {STRINGIFY(HKEY_CURRENT_USER)}.substr(pos, len)));
		inline static constexpr ULONG_PTR LocalMachine = SignExtend(ToInt32(std::string_view {STRINGIFY(HKEY_LOCAL_MACHINE)}.substr(pos, len)));

		struct KeyCloser
		{
			void operator()(HKEY key) const noexcept
			{
				Util::WarnAssertSuccess(::RegCloseKey(key), "RegCloseKey");
			}
		};

		using KeyHolder = std::unique_ptr<HKEY__, KeyCloser>;

		class RootKeyHolder
		{
			ULONG_PTR value;

		public:
			constexpr RootKeyHolder(ULONG_PTR value)
				: value(value)
			{}

			HKEY get() const
			{
				return Util::Detail::WindowsCast<HKEY>(value);
			}
		};

		template <typename T>
		BYTE * ToBytes(T * value)
		{
			return Util::Detail::WindowsCast<BYTE *>(value);
		}

		template <typename T>
		const BYTE * ToBytes(const T * value)
		{
			return Util::Detail::WindowsCast<const BYTE *>(value);
		}

		inline bool Found(LSTATUS result, std::string_view message)
		{
			if (result == ERROR_FILE_NOT_FOUND)
			{
				return false;
			}
			return Util::AssertSuccess(result, message);
		}

		inline void SetString(HKEY key, std::string_view name, std::string_view value, DWORD type)
		{
			auto wideName = Cvt::a2w(name);
			auto wideValue = Cvt::a2w(value);
			auto valueSize = static_cast<DWORD>((wideName.size() + 1) * sizeof(wchar_t));
			const auto * valueBytes = Detail::ToBytes(wideValue.c_str());
			Util::AssertSuccess(::RegSetValueExW(key, wideName.c_str(), 0, type, valueBytes, valueSize), "RegSetValueEx");
		}

		inline std::string GetString(HKEY key, const std::wstring & valueName)
		{
			DWORD size {};
			DWORD flags = static_cast<DWORD>(RRF_RT_REG_SZ) | static_cast<DWORD>(RRF_RT_REG_EXPAND_SZ);
			LSTATUS result = ::RegGetValueW(key, nullptr, valueName.c_str(), flags, nullptr, nullptr, &size);
			Util::AssertSuccess(result, "RegGetValue");

			GLib::Util::WideCharBuffer data;
			data.EnsureSize(size / sizeof(wchar_t));
			result = ::RegGetValueW(key, nullptr, valueName.c_str(), flags, nullptr, data.Get(), &size);
			Util::AssertSuccess(result, "RegGetValue");
			return Cvt::w2a(data.Get());
		}

		template <typename T>
		T GetScalar(HKEY key, const std::wstring & valueName)
		{
			DWORD actualTypeCode {};
			T value {};
			DWORD bytes {sizeof(T)};
			LSTATUS result = ::RegQueryValueExW(key, valueName.c_str(), nullptr, &actualTypeCode, Detail::ToBytes(&value), &bytes);
			Util::AssertSuccess(result, "RegQueryValueEx");
			return value;
		}
	}

	template <typename Key>
	class RegistryKey;

	using RootKey = RegistryKey<Detail::RootKeyHolder>;
	using SubKey = RegistryKey<Detail::KeyHolder>;

	template <typename Key>
	class RegistryKey
	{
		Key key;

	public:
		constexpr RegistryKey(Key key) noexcept
			: key(std::move(key))
		{}

		bool KeyExists(std::string_view path) const
		{
			HKEY resultKey {};
			LSTATUS result = ::RegOpenKeyW(key.get(), Cvt::a2w(path).c_str(), &resultKey);
			bool exists = Detail::Found(result, "RegOpenKey");
			if (exists)
			{
				Detail::KeyCloser()(resultKey);
			}
			return exists;
		}

		std::string GetString(std::string_view valueName) const
		{
			return Detail::GetString(key.get(), Cvt::a2w(valueName));
		}

		uint32_t GetInt32(std::string_view valueName) const
		{
			return Detail::GetScalar<uint32_t>(key.get(), Cvt::a2w(valueName));
		}

		uint64_t GetInt64(std::string_view valueName) const
		{
			return Detail::GetScalar<uint64_t>(key.get(), Cvt::a2w(valueName));
		}

		RegistryValue Get(std::string_view valueName) const
		{
			auto wideName = Cvt::a2w(valueName);
			DWORD bytes {};
			DWORD actualTypeCode {};
			LSTATUS result = ::RegQueryValueExW(key.get(), wideName.c_str(), nullptr, &actualTypeCode, nullptr, &bytes);
			Util::AssertSuccess(result, "RegQueryValueEx");

			switch (actualTypeCode)
			{
				case REG_SZ:
				case REG_EXPAND_SZ:
				{
					return Detail::GetString(key.get(), wideName);
				}

				case REG_DWORD:
				{
					return Detail::GetScalar<uint32_t>(key.get(), wideName);
				}

				case REG_QWORD:
				{
					return Detail::GetScalar<uint64_t>(key.get(), wideName);
				}

				default:;
			}
			throw std::runtime_error("Unsupported type");
		}

		void SetString(std::string_view name, std::string_view value)
		{
			Detail::SetString(key.get(), name, value, REG_SZ);
		}

		void SetInt32(std::string_view name, uint32_t value) const
		{
			LSTATUS result = ::RegSetValueExW(key.get(), Cvt::a2w(name).c_str(), 0, REG_DWORD, Detail::ToBytes(&value), sizeof(value));
			Util::AssertSuccess(result, "RegSetValueEx");
		}

		void SetInt64(std::string_view name, uint64_t value) const
		{
			LSTATUS result = ::RegSetValueExW(key.get(), Cvt::a2w(name).c_str(), 0, REG_QWORD, Detail::ToBytes(&value), sizeof(value));
			Util::AssertSuccess(result, "RegSetValueEx");
		}

		SubKey OpenSubKey(std::string_view path) const
		{
			HKEY subKey;
			LSTATUS result = ::RegOpenKeyExW(key.get(), Cvt::a2w(path).c_str(), 0, Detail::Read, &subKey);
			Util::AssertSuccess(result, "RegOpenKeyEx");
			return {Detail::KeyHolder {subKey}};
		}

		SubKey CreateSubKey(std::string_view path) const
		{
			HKEY subKey;
			LSTATUS result =
				::RegCreateKeyExW(key.get(), Cvt::a2w(path).c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, Detail::AllAccess, nullptr, &subKey, nullptr);
			Util::AssertSuccess(result, "RegCreateKeyEx");
			return {Detail::KeyHolder {subKey}};
		}

		bool DeleteSubKey(const std::string & path) const
		{
			return Detail::Found(::RegDeleteTreeW(key.get(), Cvt::a2w(path).c_str()), "RegDeleteKey");
		}
	};

	template <typename T>
	inline auto operator/(const RegistryKey<T> & key, std::string_view name)
	{
		return key.OpenSubKey(name);
	}

	template <typename T>
	inline RegistryValue operator&(const RegistryKey<T> & key, std::string_view name)
	{
		return key.Get(name);
	}

	namespace RegistryKeys
	{
		// STRICT means HKEY_CLASSES_ROOT etc have value of struct Key__* and cant be used in constexpr, so copying definitions
		inline static constexpr RegistryKey ClassesRoot = RootKey(Detail::ClassesRoot);
		inline static constexpr RegistryKey CurrentUser = RootKey(Detail::CurrentUser);
		inline static constexpr RegistryKey LocalMachine = RootKey(Detail::LocalMachine);
	}
}