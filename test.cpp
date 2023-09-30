#include "test.h"

namespace test {

	const std::filesystem::path test_directory = "tests/";
	const std::vector<std::filesystem::path> test_list = {
		"math.txt",
		"mod.txt",
		"basic.txt",
		"pow.txt",
		"basic.txt",
		"cpy.txt",
		"del.txt",
		"label.txt",
		"cmp.txt",
		"list.txt",
		"forloop.txt",
		"p_shift_1.txt",
		"p_shift_2.txt",
		"endfile_label.txt",
		"p_shift_3.txt",
		"list_p_shift.txt",
		"if_p_shift.txt",
		"del_p_dst.txt",
		"forloop_2.txt",
		"set_1.txt",
		"set_2.txt",
		"repl_1.txt",
		"repl_2.txt",
		"get.txt",
		"ins_1.txt",
		"ins_2.txt",
		"factorial.txt",
		"type_parse.txt",
		"float_math.txt",
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
		std::string program_text = utils::file_to_str(test_path);
		std::string correct_results_str = program_text.substr(1, program_text.find('\n') - 1);
		std::vector<bool> approx_flags = get_approx_flags(correct_results_str);
		remove_approx_flags(correct_results_str);
		std::vector<Token> correct_results = Token::str_to_tokens(correct_results_str);
		Program program(program_text);
		std::vector<Token> actual_results = program.execute();
		bool passed = compare_results(actual_results, correct_results, approx_flags);
		actual_results_p = actual_results;
		correct_results_p = correct_results;
		return passed;
	}

	void run_tests() {
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
			if (!std::filesystem::exists(test_path)) {
				throw std::runtime_error(test_path.string() + " not found");
			}
			if (!std::filesystem::is_regular_file(test_path)) {
				throw std::runtime_error(test_path.string() + " is not a file");
			}
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
	}

}