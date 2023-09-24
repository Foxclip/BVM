#include "algorithm.h"

void throwUnexpectedCharException(char c, std::string current_word) {
	throw std::runtime_error("Current word: " + current_word + ", unexpected char: " + std::string(1, c));
}

Token::Token() {}

Token::Token(long index, std::string str, long num_value) {
	this->index = index;
	this->str = str;
	this->num_value = num_value;
}

std::string Token::to_string() {
	if (str == "val") {
		if (pointer) {
			return "p(" + std::to_string(num_value) + ")";
		} else  {
			return std::to_string(num_value);
		}
	} else {
		return str;
	}
}

bool operator==(const Token& first, const Token& second) {
	return first.str == second.str;
}

Label::Label(std::string str, long token_index) {
	this->str = str;
	this->token_index = token_index;
}

std::vector<Token> Program::tokenize(std::string str) {
	std::vector<std::string> words;
	std::vector<Token> tokens;
	std::vector<Label> labels;
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
			} else if (current_char == ':') {
				labels.push_back(Label(current_word, words.size()));
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

	// creating tokens from words
	for (int i = 0; i < words.size(); i++) {
		std::string str = words[i];
		if (isNumber(str)) {
			Token new_token = Token(i, "val", std::stol(str));
			tokens.push_back(new_token);
		} else {
			Token new_token = Token(i, words[i], 0);
			tokens.push_back(new_token);
		}
	}
	
	// replacing labels with addresses
	for (int i = 0; i < tokens.size(); i++) {
		Token& current_token = tokens[i];
		auto it = std::find_if(labels.begin(), labels.end(),
			[&](Label& label) {
				return label.str == current_token.str;
			}
		);
		if (it != labels.end()) {
			long relative_address = (*it).token_index - i;
			tokens[i].str = "val";
			tokens[i].num_value = relative_address;
			tokens[i].pointer = true;
		}
	}
	return tokens;
}

Node::Node(Token token) {
	this->token = token;
}

std::string Node::to_string() {
	return token.to_string();
}

std::vector<Token> Node::tokenize() {
	std::vector<Token> tokens;
	tokens.push_back(token);
	for (int i = 0; i < arguments.size(); i++) {
		std::vector<Token> arg_tokens = arguments[i].get()->tokenize();
		tokens.insert(tokens.end(), arg_tokens.begin(), arg_tokens.end());
	}
	return tokens;
}

Program::Program(std::string str) {
	tokens = tokenize(str);
}

void Program::print_tokens() {
	for (int i = 0; i < tokens.size(); i++) {
		if (i == program_counter) {
			std::cout << "*";
		}
		std::cout << tokens[i].to_string() << " ";
	}
	if (program_counter == tokens.size()) {
		std::cout << "*";
	}
	std::cout << "\n";
}

void Program::print_nodes() {
	parse();
	for (int i = 0; i < nodes.size(); i++) {
		print_node(nodes[i].get(), 0);
	}
}

std::vector<long> Program::execute() {
	if (tokens.size() == 0) {
		throw std::runtime_error("Empty program");
	}
	prev_tokens = tokens;
	if (print_iterations) {
		std::cout << "Iteration *: ";
		print_tokens();
	}
	for (unsigned long iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
		if (print_iterations) {
			std::cout << "Iteration " << iteration << ": ";
		}
		unsigned long steps = 0;
		std::stack<long> list_scope_stack;
		for (program_counter = 0; program_counter < tokens.size(); program_counter++) {
			//std::cout << "pc: " << program_counter << " | ";
			//print_tokens();
			Token current_token_read = rel_token(tokens, 0);
			Token next_token = rel_token(tokens, 1);
			if (current_token_read.str == "val") {
				// skipping
			} else if (current_token_read.str == "inp") {
				if (next_token.str == "val") {
					shift_pointers(tokens, program_counter, -1);
					long input_index = next_token.num_value;
					long input_value = inputs[input_index];
					tokens.erase(tokens.begin() + program_counter);
					rel_token(tokens, 0).str = "val";
					rel_token(tokens, 0).num_value = input_value;
					break;
				}
			} else if (current_token_read.str == "add") {
				if (binary_func([](long a, long b) { return a + b; })) {
					break;
				}
			} else if (current_token_read.str == "sub") {
				if (binary_func([](long a, long b) { return a - b; })) {
					break;
				}
			} else if (current_token_read.str == "mul") {
				if (binary_func([](long a, long b) { return a * b; })) {
					break;
				}
			} else if (current_token_read.str == "div") {
				if (binary_func([](long a, long b) { return a / b; })) {
					break;
				}
			} else if (current_token_read.str == "mod") {
				if (binary_func([](long a, long b) { return modulo(a, b); })) {
					break;
				}
			} else if (current_token_read.str == "pow") {
				if (binary_func([](long a, long b) { return pow(a, b); })) {
					break;
				}
			} else if (current_token_read.str == "cmp") {
				if (binary_func([](long a, long b) { return a == b ? 1 : 0; })) {
					break;
				}
			} else if (current_token_read.str == "lt") {
				if (binary_func([](long a, long b) { return a < b ? 1 : 0; })) {
					break;
				}
			} else if (current_token_read.str == "gt") {
				if (binary_func([](long a, long b) { return a > b ? 1 : 0; })) {
					break;
				}
			} else if (current_token_read.str == "cpy") {
				if (rel_token(tokens, 1).str == "val" && rel_token(tokens, 2).str == "val") {
					long src = rel_token(tokens, 1).num_value;
					long dst = rel_token(tokens, 2).num_value;
					long new_token_index;
					long src_index_begin = program_counter + 1 + src;
					long dst_index = program_counter + 2 + dst;
					long cpy_position = program_counter;
					std::unique_ptr<Node> node = parse_token(tokens, token_index(tokens, src_index_begin), nullptr, new_token_index);
					std::vector<Token> node_tokens = node.get()->tokenize();
					long insertion_index_old;
					if (dst_index == tokens.size()) {
						insertion_index_old = tokens.size();
					} else {
						insertion_index_old = token_index(tokens, dst_index);
					}
					long erase_index = program_counter;
					shift_pointers(tokens, erase_index, -3);
					tokens.erase(tokens.begin() + erase_index);
					tokens.erase(tokens.begin() + erase_index);
					tokens.erase(tokens.begin() + erase_index);
					long insertion_index_new = insertion_index_old;
					if (insertion_index_old > cpy_position) {
						insertion_index_new -= std::min(insertion_index_old - cpy_position, (long)3);
					}
					shift_pointers(tokens, insertion_index_new, node_tokens.size());
					tokens.insert(tokens.begin() + insertion_index_new, node_tokens.begin(), node_tokens.end());
					break;
				}
			} else if (current_token_read.str == "del") {
				if (rel_token(tokens, 1).str == "val") {
					long arg = rel_token(tokens, 1).num_value;
					long new_token_index;
					long del_index = program_counter + 1 + arg;
					long del_position = program_counter;
					std::unique_ptr<Node> node = parse_token(tokens, token_index(tokens, del_index), nullptr, new_token_index);
					std::vector<Token> node_tokens = node.get()->tokenize();
					long index_begin = token_index(tokens, del_index);
					long index_end = index_begin + node_tokens.size() - 1;
					if (index_begin != del_position + 1) {
						shift_pointers(tokens, index_begin, -(long)(node_tokens.size()));
						tokens.erase(tokens.begin() + index_begin, tokens.begin() + index_end + 1);
					}
					if (index_end < del_position) {
						program_counter -= node_tokens.size();
						shift_pointers(tokens, program_counter, -2);
						tokens.erase(tokens.begin() + program_counter);
						tokens.erase(tokens.begin() + program_counter);
					} else if (index_begin >= del_position + 1) {
						shift_pointers(tokens, program_counter, -2);
						tokens.erase(tokens.begin() + program_counter);
						tokens.erase(tokens.begin() + program_counter);
					} else if (index_begin <= del_position && index_end >= del_position + 1) {
						program_counter = del_index;
					} else {
						throw std::runtime_error("del error");
					}
					break;
				}
			} else if (current_token_read.str == "if") {
				if (rel_token(tokens, 1).str == "val") {
					long cond = rel_token(tokens, 1).num_value;
					long new_token_index;
					std::unique_ptr<Node> if_node = parse_token(tokens, token_index(tokens, program_counter), nullptr, new_token_index);
					std::vector<Token> if_node_tokens = if_node.get()->tokenize();
					Node* true_node = if_node.get()->arguments[1].get();
					Node* false_node = if_node.get()->arguments[2].get();
					std::vector<Token> true_node_tokens = true_node->tokenize();
					std::vector<Token> false_node_tokens = false_node->tokenize();
					long true_node_offset = 2;
					long false_node_offset = 2 + true_node_tokens.size();
					long true_node_index = program_counter + true_node_offset;
					long false_node_index = program_counter + false_node_offset;
					if (cond != 0) {
						shift_pointers(tokens, false_node_index, -(long)false_node_tokens.size());
						tokens.erase(tokens.begin() + false_node_index, tokens.begin() + false_node_index + false_node_tokens.size());
						shift_pointers(tokens, program_counter, -true_node_offset);
						tokens.erase(tokens.begin() + program_counter);
						tokens.erase(tokens.begin() + program_counter);
					} else {
						shift_pointers(tokens, program_counter, -false_node_offset);
						tokens.erase(tokens.begin() + program_counter, tokens.begin() + false_node_index);
					}
					break;
				}
			} else if (current_token_read.str == "list") {
				list_scope_stack.push(program_counter);
			} else if (current_token_read.str == "end") {
				long list_pos = list_scope_stack.top();
				shift_pointers(tokens, list_pos, -1);
				tokens.erase(tokens.begin() + list_pos);
				program_counter--;
				shift_pointers(tokens, program_counter, -1);
				tokens.erase(tokens.begin() + program_counter);
				program_counter = list_pos;
				list_scope_stack.pop();
				break;
			} else if (current_token_read.str == "p") {
				if (rel_token(tokens, 1).str == "val") {
					shift_pointers(tokens, program_counter, -1);
					long result = rel_token(tokens, 1).num_value;
					tokens.erase(tokens.begin() + program_counter);
					rel_token(tokens, 0).str = "val";
					rel_token(tokens, 0).num_value = result;
					rel_token(tokens, 0).pointer = true;
					break;
				}
			}
			steps++;
		}
		if (print_iterations) {
			print_tokens();
		}
		if (tokens == prev_tokens) {
			break;
		}
		prev_tokens = tokens;
	}
	std::vector<long> results;
	for (int i = 0; i < tokens.size(); i++) {
		long result = tokens[i].num_value;
		results.push_back(result);
	}
	return results;
}

void Program::parse() {
	nodes.clear();
	Node* parent_node;
	for (long token_i = 0; token_i < tokens.size(); token_i++) {
		long new_token_i;
		std::unique_ptr<Node> node = parse_token(tokens, token_i, nullptr, new_token_i);
		nodes.push_back(std::move(node));
		token_i = new_token_i;
	}
}

long Program::token_index(std::vector<Token>& token_list, long index) {
	return modulo(index, token_list.size());
}

Token& Program::get_token(std::vector<Token>& token_list, long index) {
	return token_list[token_index(token_list, index)];
}

Token& Program::rel_token(std::vector<Token>& token_list, long offset) {
	return get_token(token_list, program_counter + offset);
}

std::unique_ptr<Node> Program::parse_token(std::vector<Token>& token_list, long parse_token_index, Node* parent_node, long& new_token_index) {
	Token current_token = get_token(token_list, parse_token_index);
	current_token.index = parse_token_index;
	long num_val = 0;
	if (isNumber(current_token.str)) {
		num_val = std::stol(current_token.str);
		current_token.str = "val";
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
	if (current_token.str == "list") {
		long cur_index = parse_token_index + 1;
		while (true) {
			Token cur_token = get_token(tokens, cur_index);
			std::unique_ptr<Node> node = parse_token(token_list, token_index(token_list, cur_index), new_node_p, parse_token_index);
			new_node_p->arguments.push_back(std::move(node));
			if (cur_token.str == "end") {
				break;
			}
			cur_index = parse_token_index + 1;
		}
	} else {
		for (int arg_i = 0; arg_i < arg_count; arg_i++) {
			if (parse_token_index + 1 >= token_list.size()) {
				std::cout << "Parser: End of program reached";
				std::cout << "\n";
				break;
			}
			std::unique_ptr<Node> node = parse_token(token_list, parse_token_index + 1, new_node_p, parse_token_index);
			new_node_p->arguments.push_back(std::move(node));
		}
	}
	new_token_index = parse_token_index;
	return new_node;
}

void Program::print_node(Node* node, int indent_level) {
	std::string indent_string = "";
	for (int j = 0; j < indent_level; j++) {
		indent_string += "    ";
	}
	std::cout << indent_string << node->to_string() << "\n";
	for (int i = 0; i < node->arguments.size(); i++) {
		print_node(node->arguments[i].get(), indent_level + 1);
	}
}

bool Program::binary_func(std::function<long(long, long)> func) {
	if (rel_token(tokens, 1).str == "val" && rel_token(tokens, 2).str == "val") {
		shift_pointers(tokens, program_counter, -2);
		long val1 = rel_token(tokens, 1).num_value;
		long val2 = rel_token(tokens, 2).num_value;
		long result = func(val1, val2);
		tokens.erase(tokens.begin() + program_counter);
		tokens.erase(tokens.begin() + program_counter);
		rel_token(tokens, 0).str = "val";
		rel_token(tokens, 0).num_value = result;
		return true;
	}
	return false;
}

void Program::shift_pointers(std::vector<Token>& token_list, long pos, long offset) {
	for (long token_i = 0; token_i < token_list.size(); token_i++) {
		Token current_token = get_token(token_list, token_i);
		if (current_token.pointer) {
			long pointer_index_old = token_i;
			long pointer_dst_old = token_i + current_token.num_value;
			long pointer_index_new = pointer_index_old;
			long pointer_dst_new = pointer_dst_old;
			if (offset < 0) {
				long recalc_offset = std::clamp(pos - pointer_index_old, offset, 0L);
				if (pos <= pointer_index_old) {
					pointer_index_new += recalc_offset;
				}
			} else {
				if (pos <= pointer_index_old) {
					pointer_index_new += offset;
				}
			}
			if (offset < 0) {
				long recalc_offset = std::clamp(pos - pointer_dst_old, offset, 0L);
				if (pos <= pointer_dst_old) {
					pointer_dst_new += recalc_offset;
				}
			} else {
				if (pos <= pointer_dst_old) {
					pointer_dst_new += offset;
				}
			}
			long new_pointer = pointer_dst_new - pointer_index_new;
			get_token(token_list, token_i).num_value = new_pointer;
		}
	}
}

InstructionDef Program::get_instruction_info(std::string token) {
	auto it = std::find_if(INSTRUCTION_LIST.begin(), INSTRUCTION_LIST.end(),
		[&](InstructionDef def) {
			return def.first == token;
		}
	);
	return *it;
}
