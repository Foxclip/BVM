#include "utils.h"

std::string fileToStr(std::string path) {
	if (!std::filesystem::exists(path)) {
		throw std::format("File not found: {}", path);
	}
	std::ifstream t(path);
	std::stringstream buffer;
	buffer << t.rdbuf();
	return buffer.str();
}

long modulo(long a, long b) {
	return (a % b + b) % b;
}

bool isNumberPrefix(char c) {
	return c == '-' || isdigit(c);
}

bool isNumber(std::string str) {
	return isNumberPrefix(str[0]);
}