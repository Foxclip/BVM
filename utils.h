#pragma once

#include <format>
#include <filesystem>
#include <fstream>

namespace utils {

	typedef unsigned long long LongNumberType;

	constexpr std::string_view intFormatStr = "{}";
	constexpr std::string_view floatFormatStr = "{:.3f}";
	template<typename T>
	std::string vector_to_str(std::vector<T> vec) {
		std::string str = "{ ";
		for (LongNumberType i = 0; i < vec.size(); i++) {
			str += std::format(intFormatStr, vec[i]);
			if (i < vec.size() - 1) {
				str += ", ";
			}
		}
		str += " }";
		return str;
	}

	std::string file_to_str(std::filesystem::path path);
	std::vector<std::string> split_string(std::string str, char delimeter);
	bool is_number_prefix(char c);
	bool is_float_inf_str(std::string str);
	bool is_double_inf_str(std::string str);
	bool is_inf_str(std::string str);
	bool is_float_nan_str(std::string str);
	bool is_double_nan_str(std::string str);
	bool is_nan_str(std::string str);
	bool is_number(std::string str);
	bool is_valid_word_prefix(char c);
	bool is_valid_word_middle(char c);
	bool is_int_suffix(char c);
	bool is_float_suffix(char c);
	bool is_number_suffix(char c);
	bool is_newline(char c);
	bool is_container_name(std::string str);
	std::string char_to_str(char c);
	std::string string_conv(std::string str);
	std::string replace_escape_seq(std::string str);
	bool alphanum_less(std::string str1, std::string str2);
	std::vector<std::filesystem::path> list_directory(std::filesystem::path path, bool alphanum = false);

	template <typename T>
	T mod(T a, T b) {
		return std::fmod((std::fmod(a, b) + b), b);
	}

}