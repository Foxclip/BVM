#include <vector>
#include <functional>
#include <string>
#include <stack>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <format>

const long int MAX_PROGRAM_STEPS = 10;
typedef std::pair<std::string, int> InstructionDef;
const std::vector<InstructionDef> INSTRUCTION_LIST = {
	std::pair("Val", 0),
	std::pair("Inp", 1),
	std::pair("Add", 2),
	std::pair("Mul", 2),
	std::pair("Cpy", 1),
};

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

class Node {
public:
	std::string string_token;
	int instruction_index;
	long num_value;
	std::vector<std::unique_ptr<Node>> arguments;

	Node(std::string string_token, int instruction_index, long num_value) {
		this->string_token = string_token;
		this->instruction_index = instruction_index;
		this->num_value = num_value;
	}

	std::string to_string() {
		if (string_token == "Val") {
			return std::to_string(num_value);
		}
		return string_token;
	}
};

class Program {
public:
	std::vector<std::string> tokens;
	std::vector<std::unique_ptr<Node>> nodes;

	Program(std::string str) {
		tokens = tokenize(str);
	}

	void print_tokens() {
		for (int i = 0; i < tokens.size(); i++) {
			std::cout << tokens[i] << " ";
		}
		std::cout << "\n";
	}

	void print_nodes() {
		for (int i = 0; i < nodes.size(); i++) {
			print_node(nodes[i].get(), 0);
		}
	}

	long int execute() {
		if (tokens.size() == 0) {
			throw std::runtime_error("Empty program");
		}
		long iteration = 0;
		bool changed = false;
		do {
			changed = false;
			print_tokens();
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

	void parse() {
		Node* parent_node;
		for (long token_i = 0; token_i < tokens.size(); token_i++) {
			long new_token_i = parse_token(0, nullptr);
			token_i = new_token_i;
		}
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

	long parse_token(long token_index, Node* parent_node) {
		std::string current_token = tokens[token_index];
		long num_val = 0;
		if (isdigit(current_token[0])) {
			num_val = std::stol(current_token);
			current_token = "Val";
		}
		auto it = std::find_if(INSTRUCTION_LIST.begin(), INSTRUCTION_LIST.end(),
			[&](InstructionDef def) {
				return def.first == current_token;
			}
		);
		if (it == INSTRUCTION_LIST.end()) {
			throw std::runtime_error("Unexpected token: " + current_token);
		}
		int instruction_index = it - INSTRUCTION_LIST.begin();
		std::unique_ptr<Node> new_node = std::make_unique<Node>(current_token, instruction_index, num_val);
		Node* new_node_p = new_node.get();
		if (parent_node) {
			parent_node->arguments.push_back(std::move(new_node));
		} else {
			nodes.push_back(std::move(new_node));
		}
		int arg_count = (*it).second;
		for (int arg_i = 0; arg_i < arg_count; arg_i++) {
			long new_token_index = parse_token(token_index + 1, new_node_p);
			token_index = new_token_index;
		}
		return token_index;
	}

	void print_node(Node* node, int indent_level) {
		std::string indent_string = "";
		for (int j = 0; j < indent_level; j++) {
			indent_string += "    ";
		}
		std::cout << indent_string << node->to_string() << "\n";
		for (int i = 0; i < node->arguments.size(); i++) {
			print_node(node->arguments[i].get(), indent_level + 1);
		}
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
		program.parse();
		std::cout << "Nodes:";
		std::cout << "\n";
		program.print_nodes();
		//long int result = program.execute();
		//std::cout << "Result: " << result << "\n";
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