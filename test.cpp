#include "test.h"

namespace test {

	struct TestCase {
		std::filesystem::path path;
		std::vector<std::string> correct_results_str;
	};

	const std::filesystem::path test_directory = "tests/";
	const std::vector<TestCase> test_list = {
		{ "math.txt", { "0", "2", "4", "0", "0", "3", "-1", "4", "1", "0", "2", "-1", "1", "0" }},
		{ "mod.txt", { "2", "0", "2", "0", "-4", "-1" } },
		{ "basic.txt", { "216", "4785" } },
	};

	bool is_terminating_char(char c) {
		return c == '\n' || c == '\r' || c == EOF;
	}

	bool is_space_char(char c) {
		return isspace(c) && !is_terminating_char(c);
	}

	bool run_test(TestCase test_case, std::vector<Token>& actual_results_p, std::vector<Token>& correct_results_p) {
		std::filesystem::path test_path = test_directory / test_case.path;
		std::string program_text = utils::file_to_str(test_path);
		std::vector<Token> correct_results;
		for (ProgramCounterType i = 0; i < test_case.correct_results_str.size(); i++) {
			Token token(test_case.correct_results_str[i]);
			correct_results.push_back(token);
		}
		Program program(program_text);
		std::vector<Token> actual_results = program.execute();
		bool passed = actual_results == correct_results;
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
		for (TestCase test_case : test_list) {
			std::filesystem::path test_path = test_directory / test_case.path;
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
				passed = run_test(test_case, actual_results, correct_results);
			} catch (std::exception exc) {
				exception = true;
				exc_message = exc.what();
			}
			std::string filename = test_case.path.string();
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