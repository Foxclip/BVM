#include "test.h"

namespace test {

	const std::filesystem::path test_directory = "tests/";
	const std::vector<std::filesystem::path> test_list = {
		"math.bvmi",
		"mod.bvmi",
		"basic.bvmi",
		"pow.bvmi",
		"basic.bvmi",
		"cpy.bvmi",
		"del.bvmi",
		"label.bvmi",
		"cmp.bvmi",
		"list.bvmi",
		"forloop.bvmi",
		"p_shift_1.bvmi",
		"p_shift_2.bvmi",
		"p_shift_3.bvmi",
		"list_p_shift.bvmi",
		"if_p_shift.bvmi",
		"del_p_dst.bvmi",
		"forloop_2.bvmi",
		"set_1.bvmi",
		"set_2.bvmi",
		"repl_1.bvmi",
		"repl_2.bvmi",
		"get.bvmi",
		"ins_1.bvmi",
		"ins_2.bvmi",
		"factorial.bvmi",
		"type_parse.bvmi",
		"float_math.bvmi",
		"inf_nan_parse.bvmi",
		"div_zero.bvmi",
		"float_mod.bvmi",
		"float_inf_nan_1.bvmi",
		"float_inf_nan_2.bvmi",
		"float_inf_nan_3.bvmi",
		"float_cmp.bvmi",
		"math_log_trig.bvmi",
		"float_inf_nan_4.bvmi",
		"floor_ceil.bvmi",
		"logic.bvmi",
		"type_strings.bvmi",
		"cast.bvmi",
		"cast_int32.bvmi",
		"cast_float.bvmi",
		"string_literal.bvmi",
	};

	bool is_terminating_char(char c) {
		return c == '\n' || c == '\r' || c == EOF;
	}

	bool is_space_char(char c) {
		return isspace(c) && !is_terminating_char(c);
	}

	bool compare_results(const std::vector<Token>& vec1, const std::vector<Token>& vec2, const std::vector<bool>& approx_flags) {
		if (vec1.size() != vec2.size()) {
			return false;
		}
		for (int i = 0; i < vec1.size(); i++) {
			const Token& token1 = vec1[i];
			const Token& token2 = vec2[i];
			const bool approx_flag = approx_flags[i];
			if (approx_flag) {
				if (!approx_compare(token1, token2, 0.0001)) {
					return false;
				}
			} else {
				if (token1 != token2) {
					return false;
				}
			}
		}
		return true;
	}

	std::vector<bool> get_approx_flags(std::string str) {
		std::vector<bool> results;
		std::vector<std::string> strings = utils::split_string(str, ' ');
		for (int i = 0; i < strings.size(); i++) {
			std::string current_str = strings[i];
			bool flag = current_str[0] == '?';
			results.push_back(flag);
		}
		return results;
	}

	void remove_approx_flags(std::string& str) {
		std::vector<std::string> strings = utils::split_string(str, ' ');
		str = "";
		for (int i = 0; i < strings.size(); i++) {
			std::string current_str = strings[i];
			if (current_str[0] == '?') {
				str += current_str.substr(1, current_str.size() - 1);
			} else {
				str += current_str;
			}
			if (i < strings.size() - 1) {
				str += " ";
			}
		}
	}

	bool run_test(std::filesystem::path test_path, std::vector<Token>& actual_results_p, std::vector<Token>& correct_results_p) {
		if (!std::filesystem::exists(test_path)) {
			throw std::runtime_error(test_path.string() + " not found");
		}
		if (!std::filesystem::is_regular_file(test_path)) {
			throw std::runtime_error(test_path.string() + " is not a file");
		}
		std::string program_text = utils::file_to_str(test_path);
		std::string correct_results_str = program_text.substr(1, program_text.find('\n') - 1);
		std::vector<bool> approx_flags = get_approx_flags(correct_results_str);
		remove_approx_flags(correct_results_str);
		std::vector<Token> correct_results;
		try {
			correct_results = Token::str_to_tokens(correct_results_str);
		} catch (std::exception exc) {
			throw std::runtime_error("Cannot parse correct results: " + std::string(exc.what()));
		}
		Program program(program_text);
		std::vector<Token> actual_results;
		try {
			actual_results = program.execute();
		} catch (std::exception exc) {
			throw std::runtime_error("Program execution error: " + std::string(exc.what()));
		}
		bool passed = compare_results(actual_results, correct_results, approx_flags);
		actual_results_p = actual_results;
		correct_results_p = correct_results;
		return passed;
	}

	void run_tests() {
		try {
			if (!std::filesystem::exists(test_directory)) {
				throw std::runtime_error(test_directory.string() + " not found");
			}
			if (!std::filesystem::is_directory(test_directory)) {
				throw std::runtime_error(test_directory.string() + " is not a directory");
			}
			std::cout << "Running tests in " << test_directory << "\n";
			int passed_count = 0;
			std::vector<std::string> failed_list;
			for (std::filesystem::path test_filename : test_list) {
				std::filesystem::path test_path = test_directory / test_filename;
				std::vector<Token> actual_results;
				std::vector<Token> correct_results;
				bool passed = false;
				bool exception = false;
				std::string exc_message;
				try {
					passed = run_test(test_path, actual_results, correct_results);
				} catch (std::exception exc) {
					exception = true;
					exc_message = exc.what();
				}
				std::string filename = test_filename.string();
				if (passed) {
					passed_count++;
					std::cout << "    passed: " << filename << "\n";
				} else {
					failed_list.push_back(filename);
					std::cout << "    FAILED: " << filename << "\n";
					if (exception) {
						std::cout << "        EXCEPTION: " << exc_message << "\n";
					} else {
						std::cout << "        Correct results: " + Token::tokens_to_str(correct_results) << "\n";
						std::cout << "         Actual results: " + Token::tokens_to_str(actual_results) << "\n";
					}
				}
			}
			std::cout << "\n";
			std::cout << "Passed " << passed_count << " tests, failed " << failed_list.size() << " tests";
			if (failed_list.size() > 0) {
				std::cout << ":";
			}
			std::cout << "\n";
			for (int i = 0; i < failed_list.size(); i++) {
				std::cout << "    " << failed_list[i] << "\n";
			}
		} catch (std::exception exc) {
			throw std::runtime_error(__FUNCTION__": " + std::string(exc.what()));
		}
	}

}