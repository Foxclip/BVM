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

	std::string file_to_str(std::string path);
	bool is_number_prefix(char c);
	bool is_number(std::string str);
	bool is_valid_word_prefix(char c);
	bool is_valid_word_middle(char c);
	bool is_number_literal(char c);
	bool alphanum_less(std::string str1, std::string str2);
	std::vector<std::filesystem::path> list_directory(std::filesystem::path path, bool alphanum = false);

	template <typename T>
	T mod(T a, T b) {
		return (a % b + b) % b;
	}

}