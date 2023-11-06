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

	bool is_float_inf_str(std::string str) {
		return str == "inff" || str == "-inff";
	}

	bool is_double_inf_str(std::string str) {
		return str == "inf" || str == "-inf";
	}

	bool is_inf_str(std::string str) {
		return is_float_inf_str(str) || is_double_inf_str(str);
	}

	bool is_float_nan_str(std::string str) {
		return str == "nanf" || str == "-nanf" || str == "-nan(ind)f";
	}

	bool is_double_nan_str(std::string str) {
		return str == "nan" || str == "-nan" || str == "-nan(ind)";
	}

	bool is_nan_str(std::string str) {
		return is_float_nan_str(str) || is_double_nan_str(str);
	}

	bool is_number(std::string str) {
		enum TokenizerState {
			STATE_BEGIN,
			STATE_BEFOREPOINT,
			STATE_AFTERPOINT,
			STATE_MANTISSA,
			STATE_MANTISSA_SIGN,
		};
		TokenizerState state = STATE_BEGIN;
		std::string str_orig = str;
		int before_point_count = 0;
		int after_point_count = 0;
		int mantissa_count = 0;
		str += EOF;
		for (int i = 0; i < str.size(); i++) {
			char current_char = str[i];
			switch (state) {
				case STATE_BEGIN:
					if (is_number_prefix(current_char)) {
						if (isdigit(current_char)) {
							before_point_count++;
						}
						state = STATE_BEFOREPOINT;
					} else if (current_char == 'i') {
						return is_inf_str(str_orig);
					} else if (current_char == 'n') {
						return is_nan_str(str_orig);
					} else {
						return false;
					}
					break;
				case STATE_BEFOREPOINT:
					if (isdigit(current_char)) {
						before_point_count++;
					} else if (current_char == 'i') {
						return is_inf_str(str_orig);
					} else if (current_char == 'n') {
						return is_nan_str(str_orig);
					} else if (current_char == '.') {
						state = STATE_AFTERPOINT;
					} else if (is_int_suffix(current_char)
						    || is_float_suffix(current_char)) {
						return i == str_orig.size() - 1;
					} else if (current_char == EOF) {
						return before_point_count > 0;
					} else {
						return false;
					}
					break;
				case STATE_AFTERPOINT:
					if (isdigit(current_char)) {
						after_point_count++;
					} else if (current_char == 'e') {
						state = STATE_MANTISSA_SIGN;
					} else if (is_float_suffix(current_char)) {
						return i == str_orig.size() - 1;
					} else if (current_char == EOF) {
						return true;
					} else {
						return false;
					}
					break;
				case STATE_MANTISSA_SIGN:
					if (current_char == '+' || current_char == '-') {
						state = STATE_MANTISSA;
					} else {
						return false;
					}
					break;
				case STATE_MANTISSA:
					if (isdigit(current_char)) {
						mantissa_count++;
					} else if (is_number_suffix(current_char)) {
						return i == str_orig.size() - 1;
					} else if (current_char == EOF) {
						return true;
					} else {
						return false;
					}
					break;
			}
		}
		return false;
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

	bool is_container_name(std::string str) {
		return str == "list" || str == "seq" || str == "ulist" || str == "useq";
	}

	std::string char_to_str(char c) {
		if (c == '\n') {
			return "\\n";
		} else if (c == '\r') {
			return "\\r";
		} else if (c == '\t') {
			return "\\t";
		} else if (c == '\0') {
			return "\\0";
		} else if (c == '\\') {
			return "\\\\";
		} else if (c == '"') {
			return "\\\"";
		} else {
			return std::string(1, c);
		}
	}

	std::string string_conv(std::string str) {
		std::string result;
		for (int i = 0; i < str.size(); i++) {
			char current_char = str[i];
			result += char_to_str(current_char);
		}
		return result;
	}

	std::string replace_escape_seq(std::string str) {
		try {
			std::string result;
			enum TokenizerState {
				STATE_NORMAL,
				STATE_ESCAPE,
			};
			TokenizerState state = STATE_NORMAL;
			std::string str_orig = str;
			str += EOF;
			for (int i = 0; i < str.size(); i++) {
				char current_char = str[i];
				switch (state) {
					case STATE_NORMAL:
						if (current_char == '\\') {
							state = STATE_ESCAPE;
						} else if (current_char == EOF) {
							break;
						} else {
							result += current_char;
						}
						break;
					case STATE_ESCAPE:
						if (current_char == 'n') {
							result += "\n";
							state = STATE_NORMAL;
						} else if (current_char == 'r') {
							result += "\r";
							state = STATE_NORMAL;
						} else if (current_char == 't') {
							result += "\t";
							state = STATE_NORMAL;
						} else if (current_char == '\\') {
							result += "\\";
							state = STATE_NORMAL;
						} else {
							throw std::runtime_error("Unexpected char: " + char_to_str(current_char));
						}
						break;
				}
			}
			return result;
		} catch (std::exception exc) {
			throw std::runtime_error(__FUNCTION__": " + std::string(exc.what()));
		}
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