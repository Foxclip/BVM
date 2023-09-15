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

constexpr std::string_view intFormatStr = "{}";
constexpr std::string_view floatFormatStr = "{:.3f}";
template<typename T>
std::string vector_to_str(std::vector<T> vec) {
	std::string str = "{ ";
	for (int i = 0; i < vec.size(); i++) {
		str += std::format(intFormatStr, vec[i]);
		if (i < vec.size() - 1) {
			str += ", ";
		}
	}
	str += " }";
	return str;
}

std::string fileToStr(const char* path) {
	if (!std::filesystem::exists(path)) {
		throw std::format("File not found: {}", path);
	}
	std::ifstream t(path);
	std::stringstream buffer;
	buffer << t.rdbuf();
	return buffer.str();
}

long neg_mod(long a, long b) {
	return (a % b + b) % b;
}

bool isNumberPrefix(char c) {
	return c == '-' || isdigit(c);
}

bool isNumber(std::string str) {
	return isNumberPrefix(str[0]);
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
		COMMENT,
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
			} else if (current_char == '#') {
				words.push_back(current_word);
				current_word = "";
				state = COMMENT;
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
			} else if (isNumberPrefix(current_char)) {
				current_word = "";
				current_word += current_char;
				state = NUM;
			} else if (current_char == '#') {
				state = COMMENT;
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
			} else if (current_char == '#') {
				words.push_back(current_word);
				current_word = "";
				state = COMMENT;
			} else if (current_char == EOF) {
				words.push_back(current_word);
				break;
			} else {
				throwUnexpectedCharException(current_char, current_word);
			}
		} else if (state == COMMENT) {
			if (current_char == '\n' || current_char == '\r') {
				state = SPACE;
			} else if (current_char == EOF) {
				break;
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

	std::vector<std::string> tokenize() {
		std::vector<std::string> tokens;
		tokens.push_back(to_string());
		for (int i = 0; i < arguments.size(); i++) {
			std::vector<std::string> arg_tokens = arguments[i].get()->tokenize();
			tokens.insert(tokens.end(), arg_tokens.begin(), arg_tokens.end());
		}
		return tokens;
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

	std::vector<long> execute() {
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
				if (isNumber(current_token)) {
					// skipping
				} else if (current_token == "Inp") {
					if (isNumber(next_token)) {
						long input_index = std::stol(next_token);
						long input_value = inputs[input_index];
						tokens.erase(tokens.begin() + program_counter);
						get_token(program_counter) = std::to_string(input_value);
						changed = true;
					}
				} else if (current_token == "Add") {
					if (isNumber(rel_token(1)) && isNumber(rel_token(2))) {
						long val1 = std::stol(rel_token(1));
						long val2 = std::stol(rel_token(2));
						long result = val1 + val2;
						tokens.erase(tokens.begin() + program_counter);
						tokens.erase(tokens.begin() + program_counter);
						get_token(program_counter) = std::to_string(result);
						changed = true;
					}
				} else if (current_token == "Mul") {
					if (isNumber(rel_token(1)) && isNumber(rel_token(2))) {
						long val1 = std::stol(rel_token(1));
						long val2 = std::stol(rel_token(2));
						long result = val1 * val2;
						tokens.erase(tokens.begin() + program_counter);
						tokens.erase(tokens.begin() + program_counter);
						get_token(program_counter) = std::to_string(result);
						changed = true;
					}
				} else if (current_token == "Cpy") {
					if (isNumber(rel_token(1))) {
						long arg = std::stol(rel_token(1));
						long new_token_index;
						std::unique_ptr<Node> node = parse_token(token_index(program_counter + arg) , nullptr, new_token_index);
						std::vector<std::string> node_tokens = node.get()->tokenize();
						tokens.erase(tokens.begin() + program_counter);
						tokens.erase(tokens.begin() + program_counter);
						tokens.insert(tokens.begin() + program_counter, node_tokens.begin(), node_tokens.end());
						changed = true;
					}
				}
				program_counter++;
			}
			iteration++;
		} while (iteration < MAX_PROGRAM_STEPS && changed);
		std::vector<long> results;
		for (int i = 0; i < tokens.size(); i++) {
			long result = std::stol(tokens[i]);
			results.push_back(result);
		}
		return results;
	}

	void parse() {
		Node* parent_node;
		for (long token_i = 0; token_i < tokens.size(); token_i++) {
			long new_token_i;
			std::unique_ptr<Node> node = parse_token(token_i, nullptr, new_token_i);
			nodes.push_back(std::move(node));
			token_i = new_token_i;
		}
	}

private:
	long program_counter = 0;
	std::vector<long> inputs = { 5, 6, 7 };

	long token_index(long index) {
		return neg_mod(index, tokens.size());
	}

	std::string& get_token(long index) {
		return tokens[token_index(index)];
	}

	std::string& rel_token(long offset) {
		return get_token(program_counter + offset);
	}

	std::unique_ptr<Node> parse_token(long token_index, Node* parent_node, long& new_token_index) {
		std::string current_token = tokens[token_index];
		long num_val = 0;
		if (isNumber(current_token)) {
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
		int arg_count = (*it).second;
		for (int arg_i = 0; arg_i < arg_count; arg_i++) {
			if (token_index + 1 >= tokens.size()) {
				std::cout << "Parser: End of program reached";
				std::cout << "\n";
				break;
			}
			std::unique_ptr<Node> node = parse_token(token_index + 1, new_node_p, token_index);
			new_node_p->arguments.push_back(std::move(node));
		}
		new_token_index = token_index;
		return new_node;
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
		std::vector<long> results = program.execute();
		std::cout << "Results: " << vector_to_str(results) << "\n";
	} catch (std::string msg) {
		std::cout << "EXCEPTION: " << msg << "\n";
	} catch (std::exception exc) {
		std::cout << "EXCEPTION: " << exc.what() << "\n";
	}

	// TODO: ctype command, switches argument from command to number, and from number to command
	// If you add 1 to the command, you get next command, numbers and command are two different looping sets
	// Get(index), Set(index, value), Insert(index, value) commands
	// TODO: make a loop somehow
	// TODO: make ifs somehow

	return 0;
}