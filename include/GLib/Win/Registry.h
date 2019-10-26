#pragma once

#include "GLib/stackorheap.h"
#include "GLib/Win/ErrorCheck.h"

namespace GLib::Win
{
	using RegistryValue = std::variant<std::string, uint32_t, uint64_t>;

	namespace Detail
	{
		struct KeyCloser
		{
			void operator()(HKEY key) const noexcept
			{
				Util::WarnAssertSuccess(::RegCloseKey(key), "RegCloseKey");
			}
		};

		using KeyHolder = std::unique_ptr<HKEY__, KeyCloser>;

		inline bool Found(LSTATUS result, const char * message)
		{
			if (result == ERROR_FILE_NOT_FOUND)
			{
				return false;
			}
			Util::AssertSuccess(result, message);
			return true;
		}

		inline void SetString(const KeyHolder & key, const std::string_view & name, const std::string_view & value, DWORD type)
		{
			auto wideName = Cvt::a2w(name);
			auto wideValue = Cvt::a2w(value);
			auto valueSize = static_cast<DWORD>((wideName.size() + 1) * sizeof(wchar_t));
			auto valueBytes = reinterpret_cast<const BYTE*>(wideValue.c_str());
			Util::AssertSuccess(::RegSetValueExW(key.get(), wideName.c_str(), 0, type, valueBytes, valueSize), "RegSetValueEx");
		}

		inline std::string GetString(const KeyHolder & key, const std::wstring & valueName)
		{
			DWORD size{};
			DWORD flags = RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ;
			LSTATUS result = ::RegGetValueW(key.get(), nullptr, valueName.c_str(), flags, nullptr, nullptr, &size);
			Util::AssertSuccess(result, "RegGetValue");

			GLib::Util::WideCharBuffer data;
			data.EnsureSize(size / sizeof(wchar_t));
			result = ::RegGetValueW(key.get(), nullptr, valueName.c_str(), flags, nullptr, data.Get(), &size);
			Util::AssertSuccess(result, "RegGetValue");
			return Cvt::w2a(data.Get());
		}

		template <typename T>
		T GetScalar(const KeyHolder & key, const std::wstring & valueName, DWORD typeCode)
		{
			DWORD actualTypeCode{};
			T value{};
			DWORD bytes{sizeof(T)};
			LSTATUS result = ::RegQueryValueExW(key.get(), valueName.c_str(), nullptr, &actualTypeCode, reinterpret_cast<LPBYTE>(&value), &bytes);
			Util::AssertSuccess(result, "RegQueryValueEx");
			if (actualTypeCode != typeCode || bytes != sizeof(T))
			{
				throw std::runtime_error("Incompatible value"); // + actualTypeCode
			}
			return value;
		}
	}

	class RegistryKey
	{
		Detail::KeyHolder key;

		RegistryKey(Detail::KeyHolder && key)
			: key(move(key))
		{}

	public:
		RegistryKey(HKEY key)
			: RegistryKey(Detail::KeyHolder{key})
		{}

		bool KeyExists(const std::string_view & path) noexcept
		{
			HKEY resultKey{};
			LSTATUS result = ::RegOpenKeyW(key.get(), Cvt::a2w(path).c_str(), &resultKey);
			bool exists = Detail::Found(result, "RegOpenKey");
			if (exists)
			{
				Detail::KeyCloser()(resultKey);
			}
			return exists;
		}

		std::string GetString(const std::string_view & valueName) const
		{
			return Detail::GetString(key, Cvt::a2w(valueName));
		}

		uint32_t GetInt32(const std::string_view & valueName) const
		{
			return Detail::GetScalar<uint32_t>(key, Cvt::a2w(valueName).c_str(), REG_DWORD);
		}

		uint64_t GetInt64(const std::string_view & valueName) const
		{
			return Detail::GetScalar<uint64_t>(key, Cvt::a2w(valueName).c_str(), REG_QWORD);
		}

		RegistryValue Get(const std::string_view & valueName) const
		{
			auto wideName = Cvt::a2w(valueName);
			DWORD bytes{};
			DWORD actualTypeCode{};
			LSTATUS result = ::RegQueryValueExW(key.get(), wideName.c_str(), nullptr, &actualTypeCode, nullptr, &bytes);
			Util::AssertSuccess(result, "RegQueryValueEx");

			switch (actualTypeCode)
			{
				case REG_SZ:
				case REG_EXPAND_SZ:
				{
					return Detail::GetString(key, wideName);
				}

				case REG_DWORD:
				{
					return Detail::GetScalar<uint32_t>(key, wideName.c_str(), REG_DWORD);
				}

				case REG_QWORD:
				{
					return Detail::GetScalar<uint64_t>(key, wideName.c_str(), REG_QWORD);
				}
			}
			throw std::runtime_error("Unsupported type");
		}

		void SetString(const std::string_view & name, const std::string_view & value)
		{
			Detail::SetString(key, name, value, REG_SZ);
		}

		void SetInt32(const std::string_view & name, uint32_t value) const
		{
			LSTATUS result = ::RegSetValueExW(key.get(), Cvt::a2w(name).c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
			Util::AssertSuccess(result, "RegSetValueEx");
		}

		void SetInt64(const std::string_view & name, uint64_t value) const
		{
			LSTATUS result = ::RegSetValueExW(key.get(), Cvt::a2w(name).c_str(), 0, REG_QWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
			Util::AssertSuccess(result, "RegSetValueEx");
		}

		RegistryKey OpenSubKey(const std::string_view & path) const
		{
			HKEY subKey;
			LSTATUS result = ::RegOpenKeyExW(key.get(), Cvt::a2w(path).c_str(), 0, KEY_READ, &subKey);
			Util::AssertSuccess(result, "RegOpenKeyEx");
			return {Detail::KeyHolder{subKey}};
		}

		RegistryKey CreateSubKey(const std::string_view & path) const
		{
			HKEY subKey;
			LSTATUS result = ::RegCreateKeyExW(key.get(), Cvt::a2w(path).c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS, nullptr, &subKey, nullptr);
			Util::AssertSuccess(result, "RegCreateKeyEx");
			return {Detail::KeyHolder{subKey}};
		}

		bool DeleteSubKey(const std::string & path)
		{
			return Detail::Found(::RegDeleteTreeW(key.get(), Cvt::a2w(path).c_str()), "RegDeleteKey");
		}
	};

	inline RegistryKey operator / (const RegistryKey & key, const std::string_view & name)
	{
		return key.OpenSubKey(name);
	}

	inline RegistryValue operator & (const RegistryKey & key, const std::string_view & name)
	{
		return key.Get(name);
	}

	namespace RegistryKeys
	{
		static inline RegistryKey ClassesRoot = RegistryKey(HKEY_CLASSES_ROOT);
		static inline RegistryKey CurrentUser = RegistryKey(HKEY_CURRENT_USER);
		static inline RegistryKey LocalMachine = RegistryKey(HKEY_LOCAL_MACHINE);
	}
}