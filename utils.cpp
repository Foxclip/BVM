#include "utils.h"

namespace utils {

	std::string file_to_str(std::filesystem::path path) {
		if (!std::filesystem::exists(path)) {
			throw std::format("File not found: {}", path.string());
		}
		std::ifstream t(path);
		std::stringstream buffer;
		buffer << t.rdbuf();
		return buffer.str();
	}

	std::vector<std::string> split_string(std::string str, char delimeter) {
		std::vector<std::string> results;
		std::string current_word;
		str += EOF;
		for (int i = 0; i < str.size(); i++) {
			char c = str[i];
			if (c == delimeter || c == EOF) {
				if (current_word.size() > 0) {
					results.push_back(current_word);
				}
				current_word = "";
			} else {
				current_word += c;
			}
		}
		return results;
	}

	bool is_number_prefix(char c) {
		return c == '-' || isdigit(c);
	}

	bool is_number(std::string str) {
		return is_number_prefix(str[0]);
	}

	bool is_valid_word_prefix(char c) {
		return isalpha(c) || c == '_';
	}

	bool is_valid_word_middle(char c) {
		return isalpha(c) || isdigit(c) || c == '_';
	}

	bool is_int_suffix(char c) {
		return
			   c == 'l'
			|| c == 'u'
			|| c == 'L'
			|| c == 'U'
			|| c == 'p';
	}

	bool is_float_suffix(char c) {
		return c == 'f';
	}

	bool is_number_suffix(char c) {
		return is_int_suffix(c) || is_float_suffix(c);
	}

	bool is_newline(char c) {
		return c == '\n' || c == '\r';
	}

	LongNumberType get_prefix_number(std::string str) {
		char* end;
		LongNumberType result = std::strtoull(str.c_str(), &end, 10);
		if (end == str.c_str()) {
			result = -1;
		}
		return result;
	}

	bool alphanum_less(std::string str1, std::string str2) {
		LongNumberType prefix1 = get_prefix_number(str1);
		LongNumberType prefix2 = get_prefix_number(str2);
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