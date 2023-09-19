#pragma once

#include <format>
#include <filesystem>
#include <fstream>

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

std::string fileToStr(const char* path);

long modulo(long a, long b);

bool isNumberPrefix(char c);

bool isNumber(std::string str);