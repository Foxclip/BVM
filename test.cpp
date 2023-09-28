#include "test.h"

namespace test {

	bool is_terminating_char(char c) {
		return c == '\n' || c == '\r' || c == EOF;
	}

	bool is_space_char(char c) {
		return isspace(c) && !is_terminating_char(c);
	}

	std::vector<std::string> tokenize_correct_results(std::string str) {
		enum SplitterState {
			START,
			NUM,
			SPACE,
		};
		str += (char)EOF;
		std::vector<std::string> words;
		SplitterState state = START;
		std::string current_word = "";
		for (int i = 0; i < str.size(); i++) {
			char current_char = str[i];
			if (state == START) {
				if (current_char == '#') {
					state = SPACE;
				} else if (is_space_char(current_char)) {
					// nothing
				} else {
					throwUnexpectedCharException(current_char, current_word);
				}
			} else if (state == NUM) {
				if (isdigit(current_char)) {
					current_word += current_char;
				} else if (is_space_char(current_char)) {
					words.push_back(current_word);
					current_word = "";
					state = SPACE;
				} else if (is_terminating_char(current_char)) {
					words.push_back(current_word);
					break;
				} else {
					throwUnexpectedCharException(current_char, current_word);
				}
			} else if (state == SPACE) {
				if (utils::is_number_prefix(current_char)) {
					current_word = "";
					current_word += current_char;
					state = NUM;
				} else if (is_space_char(current_char)) {
					// nothing
				} else if (is_terminating_char(current_char)) {
					break;
				} else {
					throwUnexpectedCharException(current_char, current_word);
				}
			}
		}
		return words;
	}

	bool run_test(std::filesystem::path path, std::vector<VectorResultsType>& actual_results_p, std::vector<VectorResultsType>& correct_results_p) {
		if (!std::filesystem::exists(path)) {
			throw std::runtime_error(path.string() + " not found");
		}
		if (!std::filesystem::is_regular_file(path)) {
			throw std::runtime_error(path.string() + " is not a file");
		}
		std::string program_text = utils::file_to_str(path.string());
		std::vector<std::string> correct_results_str = tokenize_correct_results(program_text);
		std::vector<VectorResultsType> correct_results(correct_results_str.size());
		std::transform(correct_results_str.begin(), correct_results_str.end(), correct_results.begin(),
			[](std::string str) {
				return std::stol(str);
			}
		);
		Program program(program_text);
		std::vector<VectorResultsType> actual_results = program.execute();
		bool passed = actual_results == correct_results;
		actual_results_p = actual_results;
		correct_results_p = correct_results;
		return passed;
	}

	void run_tests(std::filesystem::path path) {
		if (!std::filesystem::exists(path)) {
			throw std::runtime_error(path.string() + " not found");
		}
		if (!std::filesystem::is_directory(path)) {
			throw std::runtime_error(path.string() + " is not a directory");
		}
		std::cout << "Running tests in " << path << "\n";
		std::vector<std::filesystem::path> directory_list = utils::list_directory(path, true);
		int passed_count = 0;
		std::vector<std::string> failed_list;
		for (std::filesystem::path entry : directory_list) {
			if (std::filesystem::is_regular_file(entry)) {
				std::string filename = entry.filename().string();
				std::vector<VectorResultsType> actual_results;
				std::vector<VectorResultsType> correct_results;
				bool passed = false;
				bool exception = false;
				std::string exc_message;
				try {
					passed = run_test(entry, actual_results, correct_results);
				} catch (std::exception exc) {
					exception = true;
					exc_message = exc.what();
				}
				if (passed) {
					passed_count++;
					std::cout << "    passed: " << filename << "\n";
				} else {
					failed_list.push_back(filename);
					std::cout << "    FAILED: " << filename << "\n";
					if (exception) {
						std::cout << "        EXCEPTION: " << exc_message << "\n";
					} else {
						std::cout << "        Correct results: " + utils::vector_to_str(correct_results) << "\n";
						std::cout << "         Actual results: " + utils::vector_to_str(actual_results) << "\n";
					}
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