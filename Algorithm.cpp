#include <vector>
#include <functional>
#include <string>
#include <stack>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <format>

const long int MAX_PROGRAM_STEPS = 10;

std::string fileToStr(const char* path) {
	if (!std::filesystem::exists(path)) {
		throw std::format("File not found: {}", path);
	}
	std::ifstream t(path);
	std::stringstream buffer;
	buffer << t.rdbuf();
	return buffer.str();
}

void throwUnexpectedCharException(char c, std::string current_word) {
	throw std::runtime_error("Current word: " + current_word + ", unexpected char: " + std::string(1, c));
}

std::vector<std::string> tokenize(std::string str) {
	std::vector<std::string> words;
	if (str.size() < 1) {
		return words;
	}
	enum SplitterState {
		SPACE,
		WORD,
		NUM,
	};
	str += EOF;
	SplitterState state = SPACE;
	std::string current_word = "";
	for (int i = 0; i < str.size(); i++) {
		char current_char = str[i];
		if (current_char < -1) {
			throw std::runtime_error("Invalid char: " + std::to_string(current_char));
		}
		if (state == WORD) {
			if (isalpha(current_char) || isdigit(current_char)) {
				current_word += current_char;
			} else if (isspace(current_char)) {
				words.push_back(current_word);
				current_word = "";
				state = SPACE;
			} else if (current_char == EOF) {
				words.push_back(current_word);
				break;
			} else {
				throwUnexpectedCharException(current_char, current_word);
			}
		} else if (state == SPACE) {
			if (isspace(current_char)) {
				// nothing
			} else if (isalpha(current_char)) {
				current_word = "";
				current_word += current_char;
				state = WORD;
			} else if (isdigit(current_char)) {
				current_word = "";
				current_word += current_char;
				state = NUM;
			} else if (current_char == EOF) {
				break;
			} else {
				throwUnexpectedCharException(current_char, current_word);
			}
		} else if (state == NUM) {
			if (isspace(current_char)) {
				words.push_back(current_word);
				current_word = "";
				state = SPACE;
			} else if (isalpha(current_char)) {
				throwUnexpectedCharException(current_char, current_word);
			} else if (isdigit(current_char)) {
				current_word += current_char;
			} else if (current_char == EOF) {
				words.push_back(current_word);
				break;
			} else {
				throwUnexpectedCharException(current_char, current_word);
			}
		}
	}
	return words;
}

class Program {
public:
	std::vector<std::string> tokens;

	Program(std::string str) {
		tokens = tokenize(str);
	}

	void print() {
		for (int i = 0; i < tokens.size(); i++) {
			std::cout << tokens[i] << " ";
		}
		std::cout << "\n";
	}

	long int execute() {
		if (tokens.size() == 0) {
			throw std::runtime_error("Empty program");
		}
		long iteration = 0;
		bool changed = false;
		do {
			changed = false;
			print();
			program_counter = 0;
			while (program_counter < tokens.size()) {
				std::string current_token = rel_token(0);
				std::string next_token = rel_token(1);
				if (isdigit(current_token[0])) {
					// skipping
				} else if (current_token == "Inp") {
					if (isdigit(next_token[0])) {
						long input_index = std::stol(next_token);
						long input_value = inputs[input_index];
						tokens.erase(tokens.begin() + program_counter);
						get_token(program_counter) = std::to_string(input_value);
						changed = true;
					}
				} else if (current_token == "Add") {
					if (isdigit(rel_token(1)[0]) && isdigit(rel_token(2)[0])) {
						long val1 = std::stol(rel_token(1));
						long val2 = std::stol(rel_token(2));
						long result = val1 + val2;
						tokens.erase(tokens.begin() + program_counter);
						tokens.erase(tokens.begin() + program_counter);
						get_token(program_counter) = std::to_string(result);
						changed = true;
					}
				} else if (current_token == "Mul") {
					if (isdigit(rel_token(1)[0]) && isdigit(rel_token(2)[0])) {
						long val1 = std::stol(rel_token(1));
						long val2 = std::stol(rel_token(2));
						long result = val1 * val2;
						tokens.erase(tokens.begin() + program_counter);
						tokens.erase(tokens.begin() + program_counter);
						get_token(program_counter) = std::to_string(result);
						changed = true;
					}
				} else if (current_token == "Cpy") {
					if (current_token != next_token) {
						changed = true;
					}
					get_token(program_counter) = next_token;
				}
				program_counter++;
			}
			iteration++;
		} while (iteration < MAX_PROGRAM_STEPS && changed);
		long result = std::stol(tokens[0]);
		return result;
	}

private:
	long program_counter = 0;
	std::vector<long> inputs = { 5, 6, 7 };

	std::string& get_token(long index) {
		return tokens[index % tokens.size()];
	}

	std::string& rel_token(long offset) {
		return get_token(program_counter + offset);
	}
};

int main() {
	try {
		//std::vector<std::string> tokens = tokenize("Add1 np1 00a Inp 1");
		//for (int i = 0; i < tokens.size(); i++) {
		//	std::cout << "Token " << i << ": " << tokens[i] << "\n";
		//}
		std::string program_text = fileToStr("program.txt");
		Program program(program_text);
		long int result = program.execute();
		std::cout << "Result: " << result << "\n";
	} catch (std::string msg) {
		std::cout << "EXCEPTION: " << msg << "\n";
	} catch (std::exception exc) {
		std::cout << "EXCEPTION: " << exc.what() << "\n";
	}

	// TODO: ctype command, switches argument from command to number, and from number to command
	// If you add 1 to the command, you get next command, numbers and command are two different looping sets
	// Get(index), Set(index, value), Insert(index, value) commands
	// Copy(index) command, for copying functions (useful for loops)
	// TODO: print program hierarchy
	// TODO: make a loop somehow
	// TODO: make ifs somehow

	return 0;
}