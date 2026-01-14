#pragma once

#include <cstdint>
#include <locale>
#include <string>
#include <vector>

#define WSTRING_IS_U16STRING

namespace Utilities
{
	class Converter
	{
	public:
		static auto to_string(const std::u32string& value, std::locale target_locale = std::locale("")) -> std::string;
		static auto to_string(const std::u16string& value, std::locale target_locale = std::locale("")) -> std::string;
		static auto to_string(const std::wstring& value, std::locale target_locale = std::locale("")) -> std::string;
		static auto to_string(const std::vector<uint8_t>& value) -> std::string;

		static auto to_array(const std::string& value) -> std::vector<uint8_t>;

		static auto from_base64(const std::string& value) -> std::vector<uint8_t>;
		static auto to_base64(const std::vector<uint8_t>& value) -> std::string;

#ifdef WSTRING_IS_U16STRING
	private:
		static auto convert(const std::u16string& value) -> std::wstring;
		static auto convert(const std::wstring& value) -> std::u16string;
#endif
	};
}
