#include "utils.h"

namespace utils {

	std::string file_to_str(std::string path) {
		if (!std::filesystem::exists(path)) {
			throw std::format("File not found: {}", path);
		}
		std::ifstream t(path);
		std::stringstream buffer;
		buffer << t.rdbuf();
		return buffer.str();
	}

	long mod(long a, long b) {
		return (a % b + b) % b;
	}

	bool is_number_prefix(char c) {
		return c == '-' || isdigit(c);
	}

	bool is_number(std::string str) {
		return is_number_prefix(str[0]);
	}

	long get_prefix_number(std::string str) {
		char* end;
		long result = std::strtol(str.c_str(), &end, 10);
		if (end == str.c_str()) {
			result = -1;
		}
		return result;
	}

	bool alphanum_less(std::string str1, std::string str2) {
		long prefix1 = get_prefix_number(str1);
		long prefix2 = get_prefix_number(str2);
		if (prefix1 != -1 && prefix2 != -1) {
			return prefix1 < prefix2;
		}
		return str1.compare(str2) < 0;
	}

	std::vector<std::filesystem::path> list_directory(std::filesystem::path path, bool alphanum) {
		std::vector<std::filesystem::path> result;
		auto it = std::filesystem::directory_iterator(path);
		for (auto& entry : it) {
			result.push_back(entry.path());
		}
		if (alphanum) {
			std::sort(result.begin(), result.end(), [](std::filesystem::path path1, std::filesystem::path path2) {
					return alphanum_less(path1.filename().string(), path2.filename().string());
				}
			);
		}
		return result;
	}

}