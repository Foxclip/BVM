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
		return std::to_string(num_value);
	}
	return str;
}

std::string Token::_to_string() {
	return "[" + std::to_string(index) + "]" + _to_string();
}


bool operator==(const Token& first, const Token& second) {
	return first.str == second.str;
}

Label::Label(std::string str, long token_index) {
	this->str = str;
	this->token_index = token_index;
}

std::vector<Token> tokenize(std::string str) {
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
		LABEL,
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
				words.push_back(current_word);
				current_word = "";
				state = LABEL;
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
			} else if (current_char == ':') {
				state = LABEL;
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
			} else if (current_char == ':') {
				words.push_back(current_word);
				current_word = "";
				state = LABEL;
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
		} else if (state == LABEL) {
			if (isalpha(current_char))
				current_word += current_char;
			else if (isdigit(current_char)) {
				if (current_word.size() == 0) {
					throw std::runtime_error("Label name cannot start with a digit: " + std::string(1, current_char));
				}
				current_word += current_char;
			} else if (isspace(current_char)) {
				labels.push_back(Label(current_word, words.size() - 1));
				current_word = "";
				state = SPACE;
			} else if (current_char == '#') {
				labels.push_back(Label(current_word, words.size() - 1));
				current_word = "";
				state = COMMENT;
			} else if (current_char == EOF) {
				labels.push_back(Label(current_word, words.size() - 1));
				break;
			} else {
				throwUnexpectedCharException(current_char, current_word);
			}
		}
	}
	for (int i = 0; i < labels.size(); i++) {
		Label current_label = labels[i];
		auto it = std::find_if(INSTRUCTION_LIST.begin(), INSTRUCTION_LIST.end(),
			[&](InstructionDef idef) {
				return idef.first == current_label.str;
			}
		);
		if (it != INSTRUCTION_LIST.end()) {
			throw std::runtime_error("Label name cannot be a keyword: " + current_label.str);
		}
	}
	for (int i = 0; i < words.size(); i++) {
		std::string current_word = words[i];
		auto it = std::find_if(labels.begin(), labels.end(),
			[&](Label& label) {
				return label.str == current_word;
			}
		);
		if (it != labels.end()) {
			long relative_address = (*it).token_index - i;
			words[i] = std::to_string(relative_address);
		}
	}
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
	for (int i = 0; i < nodes.size(); i++) {
		print_node(nodes[i].get(), 0);
	}
}

std::vector<long> Program::execute() {
	if (tokens.size() == 0) {
		throw std::runtime_error("Empty program");
	}
	prev_tokens = tokens;
	std::cout << "Iteration *: ";
	print_tokens();
	for (unsigned long iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
		std::cout << "Iteration " << iteration << ": ";
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
					long input_index = next_token.num_value;
					long input_value = inputs[input_index];
					shift_pointers(program_counter, -1);
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
					shift_pointers(erase_index, -3);
					tokens.erase(tokens.begin() + erase_index);
					tokens.erase(tokens.begin() + erase_index);
					tokens.erase(tokens.begin() + erase_index);
					long insertion_index_new = insertion_index_old;
					if (insertion_index_old > cpy_position) {
						insertion_index_new -= std::min(insertion_index_old - cpy_position, (long)3);
					}
					shift_pointers(insertion_index_new, node_tokens.size());
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
						shift_pointers(index_begin, -(long)(node_tokens.size()));
						tokens.erase(tokens.begin() + index_begin, tokens.begin() + index_end + 1);
					}
					if (index_end < del_position) {
						program_counter -= node_tokens.size();
						shift_pointers(program_counter, -2);
						tokens.erase(tokens.begin() + program_counter);
						tokens.erase(tokens.begin() + program_counter);
					} else if (index_begin >= del_position + 1) {
						shift_pointers(program_counter, -2);
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
					std::vector<Token> branch_node_tokens;
					Node* true_node = if_node.get()->arguments[1].get();
					Node* false_node = if_node.get()->arguments[2].get();
					Node* branch_node = cond != 0 ? true_node : false_node;
					branch_node_tokens = branch_node->tokenize();
					long index_begin = program_counter;
					long index_end = program_counter + if_node_tokens.size();
					shift_pointers(program_counter, -(long)(branch_node_tokens.size() - if_node_tokens.size()));
					tokens.erase(tokens.begin() + index_begin, tokens.begin() + index_end);
					long insertion_index = program_counter;
					tokens.insert(tokens.begin() + insertion_index, branch_node_tokens.begin(), branch_node_tokens.end());
					break;
				}
			} else if (current_token_read.str == "list") {
				list_scope_stack.push(program_counter);
			} else if (current_token_read.str == "end") {
				shift_pointers(program_counter, -1);
				tokens.erase(tokens.begin() + program_counter);
				long list_pos = list_scope_stack.top();
				shift_pointers(list_pos, -1);
				tokens.erase(tokens.begin() + list_pos);
				list_scope_stack.pop();
				program_counter--;
				break;
			}
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
	current_token.index = parse_token_index;
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
		long val1 = rel_token(tokens, 1).num_value;
		long val2 = rel_token(tokens, 2).num_value;
		long result = func(val1, val2);
		shift_pointers(program_counter, -2);
		tokens.erase(tokens.begin() + program_counter);
		tokens.erase(tokens.begin() + program_counter);
		rel_token(tokens, 0).str = "val";
		rel_token(tokens, 0).num_value = result;
		return true;
	}
	return false;
}

void Program::shift_pointers(long pos, long offset) {
	parse();
	for (long node_i = 0; node_i < nodes.size(); node_i++) {
		_shift_pointers(nodes[node_i].get(), pos, offset);
	}
}

void Program::_shift_pointers(Node* node, long pos, long offset) {
	int arg_count = get_instruction_info(node->token.str).second;
	if (node->token.str == "cpy" || node->token.str == "del") {
		for (int i = 0; i < arg_count; i++) {
			Token arg = node->arguments[i].get()->token;
			auto shift_func = [&](Token t) {
				if (t.str == "val") {
					long pointer_index_old = t.index;
					long pointer_dst_old = t.index + t.num_value;
					long pointer_index_new = pointer_index_old;
					long pointer_dst_new = pointer_dst_old;
					if (pos <= pointer_index_old) {
						pointer_index_new += offset;
					}
					if (pos <= pointer_dst_old) {
						pointer_dst_new += offset;
					}
					long new_pointer = pointer_dst_new - pointer_index_new;
					get_token(tokens, t.index).num_value = new_pointer;
				}
			};
			shift_func(arg);
		}
	}
	for (int arg_i = 0; arg_i < node->arguments.size(); arg_i++) {
		Node* arg_node = node->arguments[arg_i].get();
		_shift_pointers(arg_node, pos, offset);
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
