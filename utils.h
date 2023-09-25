#pragma once

#include <format>
#include <filesystem>
#include <fstream>

namespace utils {

	constexpr std::string_view intFormatStr = "{}";
	constexpr std::string_view floatFormatStr = "{:.3f}";
	template<typename T>
	std::string vector_to_str(std::vector<T> vec) {
		std::string str = "{ ";
		for (int i = 0; i < vec.size(); i++) {
			str += std::format(intFormatStr, vec[i]);
			if (i < vec.size() - 1) {
				str += ", ";
			}
		}
		str += " }";
		return str;
	}

	std::string file_to_str(std::string path);

	long mod(long a, long b);

	bool is_number_prefix(char c);

	bool is_number(std::string str);

	bool alphanum_less(std::string str1, std::string str2);

}