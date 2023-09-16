#include <vector>
#include <functional>
#include <string>
#include <stack>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <format>
#include <algorithm>

const long MAX_ITERATIONS = 10;
const long MAX_PROGRAM_STEPS = 100;
typedef std::pair<std::string, int> InstructionDef;
const std::vector<InstructionDef> INSTRUCTION_LIST = {
	std::pair("Val", 0),
	std::pair("Inp", 1),
	std::pair("Add", 2),
	std::pair("Mul", 2),
	std::pair("Cpy", 2),
	std::pair("Node", 2),
	std::pair("Del", 1),
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

struct Token {
	long index = 0;
	std::string str;
	long num_value = 0;

	Token() { }

	Token(long index, std::string str, long num_value) {
		this->index = index;
		this->str = str;
		this->num_value = num_value;
	}

	std::string to_string() {
		if (str == "Val") {
			return std::to_string(num_value);
		}
		return str;
	}

	std::string _to_string() {
		return "[" + std::to_string(index) + "]" + _to_string();
	}

	friend bool operator==(const Token& first, const Token& second);
};

bool operator==(const Token& first, const Token& second) {
	return first.str == second.str;
}

std::vector<Token> tokenize(std::string str) {
	std::vector<std::string> words;
	std::vector<Token> tokens;
	if (str.size() < 1) {
		return tokens;
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
	for (int i = 0; i < words.size(); i++) {
		std::string str = words[i];
		if (isNumber(str)) {
			Token new_token = Token(i, "Val", std::stol(str));
			tokens.push_back(new_token);
		} else {
			Token new_token = Token(i, words[i], 0);
			tokens.push_back(new_token);
		}
	}
	return tokens;
}

class Node {
public:
	Token token;
	std::vector<std::unique_ptr<Node>> arguments;

	Node(Token token) {
		this->token = token;
	}

	std::string to_string() {
		return token.to_string();
	}

	std::vector<Token> tokenize() {
		std::vector<Token> tokens;
		tokens.push_back(token);
		for (int i = 0; i < arguments.size(); i++) {
			std::vector<Token> arg_tokens = arguments[i].get()->tokenize();
			tokens.insert(tokens.end(), arg_tokens.begin(), arg_tokens.end());
		}
		return tokens;
	}
};

class Program {
public:
	std::vector<Token> tokens;
	std::vector<Token> prev_tokens;
	std::vector<std::unique_ptr<Node>> nodes;

	Program(std::string str) {
		tokens = tokenize(str);
	}

	void print_tokens() {
		for (int i = 0; i < tokens.size(); i++) {
			//if (i == program_counter) {
			//	std::cout << "*";
			//}
			std::cout << tokens[i].to_string() << " ";
		}
		//if (program_counter == tokens.size()) {
		//	std::cout << "*";
		//}
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
		prev_tokens = tokens;
		std::cout << "Iteration *: ";
		print_tokens();
		for (int iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
			std::cout << "Iteration " << iteration << ": ";
			program_counter = 0;
			long steps = 0;
			while (program_counter < tokens.size() && steps < MAX_PROGRAM_STEPS) {
				//std::cout << "pc: " << program_counter << " | ";
				//print_tokens();
				Token current_token_read = rel_token(tokens, 0);
				Token next_token = rel_token(tokens, 1);
				if (current_token_read.str == "Val") {
					// skipping
				} else if (current_token_read.str == "Inp") {
					if (next_token.str == "Val") {
						long input_index = next_token.num_value;
						long input_value = inputs[input_index];
						tokens.erase(tokens.begin() + program_counter);
						rel_token(tokens, 0).str = "Val";
						rel_token(tokens, 0).num_value = input_value;
						break;
					}
				} else if (current_token_read.str == "Add") {
					if (rel_token(tokens, 1).str == "Val" && rel_token(tokens, 2).str == "Val") {
						long val1 = rel_token(tokens, 1).num_value;
						long val2 = rel_token(tokens, 2).num_value;
						long result = val1 + val2;
						tokens.erase(tokens.begin() + program_counter);
						tokens.erase(tokens.begin() + program_counter);
						rel_token(tokens, 0).str = "Val";
						rel_token(tokens, 0).num_value = result;
						break;
					}
				} else if (current_token_read.str == "Mul") {
					if (rel_token(tokens, 1).str == "Val" && rel_token(tokens, 2).str == "Val") {
						long val1 = rel_token(tokens, 1).num_value;
						long val2 = rel_token(tokens, 2).num_value;
						long result = val1 * val2;
						tokens.erase(tokens.begin() + program_counter);
						tokens.erase(tokens.begin() + program_counter);
						rel_token(tokens, 0).str = "Val";
						rel_token(tokens, 0).num_value = result;
						break;
					}
				} else if (current_token_read.str == "Cpy") {
					if (rel_token(tokens, 1).str == "Val" && rel_token(tokens, 2).str == "Val") {
						long src = rel_token(tokens, 1).num_value;
						long dst = rel_token(tokens, 2).num_value;
						if (dst == 1 || dst == 2) {
							dst = 0;
						}
						long new_token_index;
						long src_index_begin = program_counter + src;
						long dst_index = program_counter + dst;
						long cpy_position = program_counter;
						std::unique_ptr<Node> node = parse_token(tokens, token_index(tokens, src_index_begin) , nullptr, new_token_index);
						std::vector<Token> node_tokens = node.get()->tokenize();
						tokens.insert(tokens.begin() + dst_index, node_tokens.begin(), node_tokens.end());
						if (dst_index <= cpy_position) {
							program_counter += node_tokens.size();
							tokens.erase(tokens.begin() + program_counter);
							tokens.erase(tokens.begin() + program_counter);
							tokens.erase(tokens.begin() + program_counter);
						} else if (dst_index > cpy_position + 2) {
							tokens.erase(tokens.begin() + program_counter);
							tokens.erase(tokens.begin() + program_counter);
							tokens.erase(tokens.begin() + program_counter);
						} else {
							throw std::runtime_error("Cpy error");
						}
						break;
					}
				} else if (current_token_read.str == "Node") {
					if (rel_token(tokens, 1).str == "Val" && rel_token(tokens, 2).str == "Val") {
						long val1 = rel_token(tokens, 1).num_value;
						long val2 = rel_token(tokens, 2).num_value;
						tokens.erase(tokens.begin() + program_counter);
						rel_token(tokens, 0).str = "Val";
						rel_token(tokens, 0).num_value = val1;
						rel_token(tokens, 1).str = "Val";
						rel_token(tokens, 1).num_value = val2;
						program_counter += 1;
						break;
					}
				} else if (current_token_read.str == "Del") {
					if (rel_token(tokens, 1).str == "Val") {
						long arg = rel_token(tokens, 1).num_value;
						long new_token_index;
						long del_index = program_counter + arg;
						long del_position = program_counter;
						std::unique_ptr<Node> node = parse_token(tokens, token_index(tokens, del_index), nullptr, new_token_index);
						std::vector<Token> node_tokens = node.get()->tokenize();
						long index_begin = token_index(tokens, del_index);
						long index_end = index_begin + node_tokens.size() - 1;
						if (arg != 1) {
							tokens.erase(tokens.begin() + index_begin, tokens.begin() + index_end + 1);
						}
						if (index_end < del_position) {
							program_counter -= node_tokens.size();
							tokens.erase(tokens.begin() + program_counter);
							tokens.erase(tokens.begin() + program_counter);
						} else if (index_begin >= del_position + 1) {
							tokens.erase(tokens.begin() + program_counter);
							tokens.erase(tokens.begin() + program_counter);
						} else if (index_begin <= del_position && index_end >= del_position + 1) {
							program_counter = del_index;
						} else {
							throw std::runtime_error("Del error");
						}
						program_counter -= 1;
						break;
					}
				}
				program_counter++;
				steps++;
			}
			print_tokens();
			if (tokens == prev_tokens) {
				break;
			}
			//for (long i = 0; i < tokens.size(); i++) {
			//	tokens[i].index = i;
			//}
			prev_tokens = tokens;
		}
		std::vector<long> results;
		for (int i = 0; i < tokens.size(); i++) {
			long result = tokens[i].num_value;
			results.push_back(result);
		}
		return results;
	}

	void parse() {
		Node* parent_node;
		for (long token_i = 0; token_i < tokens.size(); token_i++) {
			long new_token_i;
			std::unique_ptr<Node> node = parse_token(tokens, token_i, nullptr, new_token_i);
			nodes.push_back(std::move(node));
			token_i = new_token_i;
		}
	}

private:
	long program_counter = 0;
	std::vector<long> inputs = { 5, 6, 7 };

	long token_index(std::vector<Token>& token_list, long index) {
		return neg_mod(index, token_list.size());
	}

	Token& get_token(std::vector<Token>& token_list, long index) {
		return token_list[token_index(token_list, index)];
	}

	Token& rel_token(std::vector<Token>& token_list, long offset) {
		return get_token(token_list, program_counter + offset);
	}

	std::unique_ptr<Node> parse_token(std::vector<Token>& token_list, long token_index, Node* parent_node, long& new_token_index) {
		Token current_token = token_list[token_index];
		long num_val = 0;
		if (isNumber(current_token.str)) {
			num_val = std::stol(current_token.str);
			current_token.str = "Val";
		}
		auto it = std::find_if(INSTRUCTION_LIST.begin(), INSTRUCTION_LIST.end(),
			[&](InstructionDef def) {
				return def.first == current_token.str;
			}
		);
		if (it == INSTRUCTION_LIST.end()) {
			throw std::runtime_error("Unexpected token: " + current_token.str);
		}
		int instruction_index = it - INSTRUCTION_LIST.begin();
		std::unique_ptr<Node> new_node = std::make_unique<Node>(current_token);
		Node* new_node_p = new_node.get();
		int arg_count = (*it).second;
		for (int arg_i = 0; arg_i < arg_count; arg_i++) {
			if (token_index + 1 >= token_list.size()) {
				std::cout << "Parser: End of program reached";
				std::cout << "\n";
				break;
			}
			std::unique_ptr<Node> node = parse_token(token_list, token_index + 1, new_node_p, token_index);
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
	// TODO: List node, container node
	// EndList node is end of container
	// TODO: fix program counter desync between tokens and prev_tokens

	return 0;
}