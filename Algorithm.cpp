#include "algorithm.h"

void throwUnexpectedCharException(char c, std::string current_word, ProgramCounterType line) {
	throw std::runtime_error(
		"Line " + std::to_string(line)
		+ ", current word: " + current_word 
		+ ", unexpected char: " + std::string(1, c)
	);
}

Label::Label(std::string str, PointerDataType token_index) {
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
		STATE_SPACE,
		STATE_WORD,
		STATE_COMMENT,
	};
	str += EOF;
	SplitterState state = STATE_SPACE;
	std::string current_word = "";
	ProgramCounterType current_line = 1;
	for (ProgramCounterType i = 0; i < str.size(); i++) {
		char current_char = str[i];
		if (current_char < -1) {
			throw std::runtime_error("Invalid char: " + std::to_string(current_char));
		}
		if (state == STATE_WORD) {
			if (isspace(current_char)) {
				words.push_back(current_word);
				current_word = "";
				state = STATE_SPACE;
			} else if (current_char == '#') {
				words.push_back(current_word);
				current_word = "";
				state = STATE_COMMENT;
			} else if (current_char == EOF) {
				words.push_back(current_word);
				break;
			} else {
				current_word += current_char;
			}
		} else if (state == STATE_SPACE) {
			if (isspace(current_char)) {
				// ok
			} else if (current_char == '#') {
				state = STATE_COMMENT;
			} else if (current_char == EOF) {
				break;
			} else {
				current_word = "";
				current_word += current_char;
				state = STATE_WORD;
			}
		} else if (state == STATE_COMMENT) {
			if (utils::is_newline(current_char)) {
				state = STATE_SPACE;
			} else if (current_char == EOF) {
				break;
			}
		}
		if (utils::is_newline(current_char)) {
			current_line++;
		}
	}

	// insert labels

	for (ProgramCounterType i = 0; i < words.size(); i++) {
		std::string str = words[i];
		Token new_token;
		auto it = std::find_if(labels.begin(), labels.end(),
			[&](Label& label) {
				return label.str == str;
			}
		);
		if (it != labels.end()) {
			PointerDataType relative_address = (*it).token_index - i;
			new_token = Token(str, Token::get_token_type<PointerTokenType>());
			new_token.set_data<PointerDataType>(relative_address);
		} else {
			new_token = Token(str);
		}
		tokens.push_back(new_token);
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
	for (ProgramCounterType i = 0; i < arguments.size(); i++) {
		std::vector<Token> arg_tokens = arguments[i].get()->tokenize();
		tokens.insert(tokens.end(), arg_tokens.begin(), arg_tokens.end());
	}
	return tokens;
}

Program::Program(std::string str) {
	tokens = tokenize(str);
}

void Program::print_tokens(bool print_program_counter) {
	for (ProgramCounterType i = 0; i < tokens.size(); i++) {
		if (print_program_counter && i == program_counter) {
			std::cout << "*";
		}
		std::cout << tokens[i].to_string() << " ";
	}
	if (print_program_counter && program_counter == tokens.size()) {
		std::cout << "*";
	}
	std::cout << "\n";
}

void Program::print_nodes() {
	parse();
	for (ProgramCounterType i = 0; i < nodes.size(); i++) {
		print_node(nodes[i].get(), 0);
	}
}

std::vector<Token> Program::execute() {
	if (tokens.size() == 0) {
		throw std::runtime_error("Empty program");
	}
	prev_tokens = tokens;
	if (print_iterations) {
		std::cout << "Iteration *: ";
		print_tokens();
	}
	for (ProgramCounterType iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
		if (print_iterations) {
			std::cout << "Iteration " << iteration << ": ";
		}
		ProgramCounterType steps = 0;
		std::stack<ProgramCounterType> list_scope_stack;
		for (program_counter = 0; program_counter < tokens.size(); program_counter++) {
			Token current_token_read = rel_token(tokens, 0);
			Token next_token = rel_token(tokens, 1);
			if (current_token_read.is_num_or_ptr()) {
				// skipping
			} else if (current_token_read.str == "add") {
				if (binary_func([](Token a, Token b) { return Token::add(a, b); })) {
					break;
				}
			} else if (current_token_read.str == "sub") {
				if (binary_func([](Token a, Token b) { return Token::sub(a, b); })) {
					break;
				}
			} else if (current_token_read.str == "mul") {
				if (binary_func([](Token a, Token b) { return Token::mul(a, b); })) {
					break;
				}
			} else if (current_token_read.str == "div") {
				if (binary_func([](Token a, Token b) { return Token::div(a, b); })) {
					break;
				}
			} else if (current_token_read.str == "mod") {
				if (binary_func([](Token a, Token b) { return Token::mod(a, b); })) {
					break;
				}
			} else if (current_token_read.str == "pow") {
				if (binary_func([](Token a, Token b) { return Token::pow(a, b); })) {
					break;
				}
			} else if (current_token_read.str == "cmp") {
				if (binary_func([](Token a, Token b) { return Token::cmp(a, b); })) {
					break;
				}
			} else if (current_token_read.str == "lt") {
				if (binary_func([](Token a, Token b) { return Token::lt(a, b); })) {
					break;
				}
			} else if (current_token_read.str == "gt") {
				if (binary_func([](Token a, Token b) { return Token::gt(a, b); })) {
					break;
				}
			} else if (current_token_read.str == "cpy") {
				if (rel_token(tokens, 1).is_num_or_ptr() && rel_token(tokens, 2).is_num_or_ptr()) {
					PointerDataType src = rel_token(tokens, 1).get_data_cast<PointerDataType>();
					PointerDataType dst = rel_token(tokens, 2).get_data_cast<PointerDataType>();
					PointerDataType new_token_index;
					PointerDataType src_index_begin = program_counter + 1 + src;
					PointerDataType dst_index = program_counter + 2 + dst;
					PointerDataType cpy_position = program_counter;
					std::unique_ptr<Node> node = parse_token(tokens, token_index(tokens, src_index_begin), nullptr, new_token_index);
					std::vector<Token> node_tokens = node.get()->tokenize();
					PointerDataType insertion_index_old;
					if (dst_index == tokens.size()) {
						insertion_index_old = tokens.size();
					} else {
						insertion_index_old = token_index(tokens, dst_index);
					}
					PointerDataType erase_index = program_counter;
					shift_pointers(tokens, erase_index, -3);
					tokens.erase(tokens.begin() + erase_index);
					tokens.erase(tokens.begin() + erase_index);
					tokens.erase(tokens.begin() + erase_index);
					PointerDataType insertion_index_new = insertion_index_old;
					if (insertion_index_old > cpy_position) {
						insertion_index_new -= std::min(insertion_index_old - cpy_position, (PointerDataType)3);
					}
					shift_pointers(tokens, insertion_index_new, node_tokens.size());
					tokens.insert(tokens.begin() + insertion_index_new, node_tokens.begin(), node_tokens.end());
					break;
				}
			} else if (current_token_read.str == "del") {
				if (rel_token(tokens, 1).is_num_or_ptr()) {
					PointerDataType arg = rel_token(tokens, 1).get_data_cast<PointerDataType>();
					PointerDataType new_token_index;
					PointerDataType del_index = program_counter + 1 + arg;
					PointerDataType del_position = program_counter;
					std::unique_ptr<Node> node = parse_token(tokens, token_index(tokens, del_index), nullptr, new_token_index);
					std::vector<Token> node_tokens = node.get()->tokenize();
					PointerDataType index_begin = token_index(tokens, del_index);
					PointerDataType index_end = index_begin + node_tokens.size() - 1;
					if (index_begin != del_position + 1) {
						shift_pointers(tokens, index_begin, -(PointerDataType)(node_tokens.size()));
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
			} else if (current_token_read.str == "set") {
				if (rel_token(tokens, 1).is_num_or_ptr()) {
					PointerDataType dst = rel_token(tokens, 1).get_data_cast<PointerDataType>();
					PointerDataType dst_index_begin = token_index(tokens, program_counter + 1 + dst);
					PointerDataType src_index_begin = token_index(tokens, program_counter + 2);
					PointerDataType dst_last_index;
					PointerDataType repl_index = program_counter;
					PointerDataType repl_last_index;
					std::unique_ptr<Node> repl_node = parse_token(tokens, repl_index, nullptr, repl_last_index);
					if (dst_index_begin >= repl_index && dst_index_begin <= repl_last_index) {
						dst_index_begin = repl_index;
					}
					Node* src_node = repl_node.get()->arguments[1].get();
					std::vector<Token> repl_node_tokens = repl_node->tokenize();
					std::vector<Token> src_node_tokens = src_node->tokenize();
					std::unique_ptr<Node> dst_node = parse_token(tokens, token_index(tokens, dst_index_begin), nullptr, dst_last_index);
					std::vector<Token> dst_node_tokens = dst_node.get()->tokenize();
					PointerDataType insertion_index = dst_index_begin;
					if (dst_index_begin != repl_index) {
						shift_pointers(tokens, repl_index, -(PointerDataType)repl_node_tokens.size());
						tokens.erase(tokens.begin() + repl_index, tokens.begin() + repl_last_index + 1);
						if (insertion_index > repl_index) {
							insertion_index -= repl_node_tokens.size();
						}
					}
					PointerDataType pointer_offset = src_node_tokens.size() - dst_node_tokens.size();
					shift_pointers(tokens, insertion_index, pointer_offset);
					tokens.erase(tokens.begin() + insertion_index, tokens.begin() + insertion_index + dst_node_tokens.size());
					tokens.insert(tokens.begin() + insertion_index, src_node_tokens.begin(), src_node_tokens.end());
					break;
				}
			} else if (current_token_read.str == "repl") {
				if (rel_token(tokens, 1).is_num_or_ptr() && rel_token(tokens, 2).is_num_or_ptr()) {
					PointerDataType dst = rel_token(tokens, 1).get_data_cast<PointerDataType>();
					PointerDataType src = rel_token(tokens, 2).get_data_cast<PointerDataType>();
					PointerDataType dst_index_begin = token_index(tokens, program_counter + 1 + dst);
					PointerDataType src_index_begin = token_index(tokens, program_counter + 2 + src);
					PointerDataType src_last_index;
					PointerDataType dst_last_index;
					PointerDataType repl_index = program_counter;
					if (dst_index_begin >= repl_index && dst_index_begin < repl_index + 3) {
						dst_index_begin = repl_index;
					}
					std::unique_ptr<Node> src_node = parse_token(tokens, token_index(tokens, src_index_begin), nullptr, src_last_index);
					std::vector<Token> src_node_tokens = src_node.get()->tokenize();
					std::unique_ptr<Node> dst_node = parse_token(tokens, token_index(tokens, dst_index_begin), nullptr, dst_last_index);
					std::vector<Token> dst_node_tokens = dst_node.get()->tokenize();
					PointerDataType insertion_index = dst_index_begin;
					if (dst_index_begin != repl_index) {
						shift_pointers(tokens, repl_index, -3);
						tokens.erase(tokens.begin() + repl_index);
						tokens.erase(tokens.begin() + repl_index);
						tokens.erase(tokens.begin() + repl_index);
						if (insertion_index > repl_index) {
							insertion_index -= 3;
						}
					}
					PointerDataType pointer_offset = src_node_tokens.size() - dst_node_tokens.size();
					shift_pointers(tokens, insertion_index, pointer_offset);
					tokens.erase(tokens.begin() + insertion_index, tokens.begin() + insertion_index + dst_node_tokens.size());
					tokens.insert(tokens.begin() + insertion_index, src_node_tokens.begin(), src_node_tokens.end());
					break;
				}
			} else if (current_token_read.str == "get") {
				if (rel_token(tokens, 1).is_num_or_ptr()) {
					PointerDataType src = rel_token(tokens, 1).get_data_cast<PointerDataType>();
					PointerDataType src_index_begin = token_index(tokens, program_counter + 1 + src);
					PointerDataType get_index = program_counter;
					PointerDataType src_last_index;
					std::unique_ptr<Node> src_node = parse_token(tokens, token_index(tokens, src_index_begin), nullptr, src_last_index);
					std::vector<Token> src_node_tokens = src_node.get()->tokenize();
					PointerDataType pointer_offset = src_node_tokens.size() - 2;
					shift_pointers(tokens, get_index, pointer_offset);
					tokens.erase(tokens.begin() + get_index, tokens.begin() + get_index + 2);
					tokens.insert(tokens.begin() + get_index, src_node_tokens.begin(), src_node_tokens.end());
					break;
				}
			} else if (current_token_read.str == "ins") {
				if (rel_token(tokens, 1).is_num_or_ptr()) {
					PointerDataType dst = rel_token(tokens, 1).get_data_cast<PointerDataType>();
					PointerDataType dst_index = token_index(tokens, program_counter + 1 + dst);
					PointerDataType src_index_begin = token_index(tokens, program_counter + 2);
					PointerDataType ins_index = program_counter;
					PointerDataType ins_last_index;
					std::unique_ptr<Node> ins_node = parse_token(tokens, ins_index, nullptr, ins_last_index);
					std::vector<Token> ins_node_tokens = ins_node->tokenize();
					if (dst_index >= ins_index && dst_index <= ins_last_index) {
						dst_index = ins_index;
					}
					Node* src_node = ins_node.get()->arguments[1].get();
					std::vector<Token> src_node_tokens = src_node->tokenize();
					PointerDataType insertion_index = dst_index;
					shift_pointers(tokens, ins_index, -(PointerDataType)ins_node_tokens.size());
					tokens.erase(tokens.begin() + ins_index, tokens.begin() + ins_last_index + 1);
					if (insertion_index > ins_index) {
						insertion_index -= std::min(insertion_index - ins_index, (PointerDataType)ins_node_tokens.size());
					}
					PointerDataType pointer_offset = src_node_tokens.size();
					shift_pointers(tokens, insertion_index, pointer_offset);
					tokens.insert(tokens.begin() + insertion_index, src_node_tokens.begin(), src_node_tokens.end());
					break;
				}
			} else if (current_token_read.str == "if") {
				if (rel_token(tokens, 1).is_num_or_ptr()) {
					BoolType cond = rel_token(tokens, 1).get_data_cast<BoolType>();
					PointerDataType new_token_index;
					std::unique_ptr<Node> if_node = parse_token(tokens, token_index(tokens, program_counter), nullptr, new_token_index);
					std::vector<Token> if_node_tokens = if_node.get()->tokenize();
					Node* true_node = if_node.get()->arguments[1].get();
					Node* false_node = if_node.get()->arguments[2].get();
					std::vector<Token> true_node_tokens = true_node->tokenize();
					std::vector<Token> false_node_tokens = false_node->tokenize();
					PointerDataType true_node_offset = 2;
					PointerDataType false_node_offset = 2 + true_node_tokens.size();
					PointerDataType true_node_index = program_counter + true_node_offset;
					PointerDataType false_node_index = program_counter + false_node_offset;
					if (cond != 0) {
						shift_pointers(tokens, false_node_index, -(PointerDataType)false_node_tokens.size());
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
				ProgramCounterType list_pos = list_scope_stack.top();
				shift_pointers(tokens, list_pos, -1);
				tokens.erase(tokens.begin() + list_pos);
				program_counter--;
				shift_pointers(tokens, program_counter, -1);
				tokens.erase(tokens.begin() + program_counter);
				program_counter = list_pos;
				list_scope_stack.pop();
				break;
			} else if (current_token_read.str == "p") {
				if (rel_token(tokens, 1).is_num_or_ptr()) {
					shift_pointers(tokens, program_counter, -1);
					PointerDataType result = rel_token(tokens, 1).get_data_cast<PointerDataType>();
					tokens.erase(tokens.begin() + program_counter);
					rel_token(tokens, 0).str = std::to_string(result);
					rel_token(tokens, 0).set_data<PointerDataType>(result);
					rel_token(tokens, 0).type = type_ptr;
					break;
				}
			} else {
				throw std::runtime_error("Unexpected token: " + current_token_read.str);
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
	return tokens;
}

void Program::parse() {
	nodes.clear();
	Node* parent_node;
	for (PointerDataType token_i = 0; token_i < tokens.size(); token_i++) {
		PointerDataType new_token_i;
		std::unique_ptr<Node> node = parse_token(tokens, token_i, nullptr, new_token_i);
		nodes.push_back(std::move(node));
		token_i = new_token_i;
	}
}

PointerDataType Program::token_index(std::vector<Token>& token_list, PointerDataType index) {
	return utils::mod(index, (PointerDataType)token_list.size());
}

Token& Program::get_token(std::vector<Token>& token_list, PointerDataType index) {
	return token_list[token_index(token_list, index)];
}

Token& Program::rel_token(std::vector<Token>& token_list, PointerDataType offset) {
	return get_token(token_list, program_counter + offset);
}

std::unique_ptr<Node> Program::parse_token(std::vector<Token>& token_list, PointerDataType parse_token_index, Node* parent_node, PointerDataType& new_token_index) {
	Token current_token = get_token(token_list, parse_token_index);
	std::unique_ptr<Node> new_node = std::make_unique<Node>(current_token);
	Node* new_node_p = new_node.get();
	if (!current_token.is_num_or_ptr()) {
		auto it = std::find_if(INSTRUCTION_LIST.begin(), INSTRUCTION_LIST.end(),
			[&](InstructionDef def) {
				return def.str == current_token.str;
			}
		);
		if (it == INSTRUCTION_LIST.end()) {
			throw std::runtime_error("Unexpected token: " + current_token.str);
		}
		int instruction_index = it - INSTRUCTION_LIST.begin();
		int arg_count = (*it).arg_count;
		if (current_token.str == "list") {
			PointerDataType cur_index = parse_token_index + 1;
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
	for (PointerDataType i = 0; i < node->arguments.size(); i++) {
		print_node(node->arguments[i].get(), indent_level + 1);
	}
}

void Program::shift_pointers(std::vector<Token>& token_list, PointerDataType pos, PointerDataType offset) {
	for (ProgramCounterType token_i = 0; token_i < token_list.size(); token_i++) {
		Token current_token = get_token(token_list, token_i);
		if (current_token.is_ptr()) {
			PointerDataType pointer_index_old = token_i;
			PointerDataType pointer_dst_old = token_i + current_token.get_data_cast<PointerDataType>();
			PointerDataType pointer_index_new = pointer_index_old;
			PointerDataType pointer_dst_new = pointer_dst_old;
			if (offset < 0) {
				PointerDataType recalc_offset = std::clamp(pos - pointer_index_old, offset, (PointerDataType)0);
				if (pos <= pointer_index_old) {
					pointer_index_new += recalc_offset;
				}
			} else {
				if (pos <= pointer_index_old) {
					pointer_index_new += offset;
				}
			}
			if (offset < 0) {
				PointerDataType recalc_offset = std::clamp(pos - pointer_dst_old, offset, (PointerDataType)0);
				if (pos <= pointer_dst_old) {
					pointer_dst_new += recalc_offset;
				}
			} else {
				if (pos <= pointer_dst_old) {
					pointer_dst_new += offset;
				}
			}
			PointerDataType new_pointer = pointer_dst_new - pointer_index_new;
			get_token(token_list, token_i).set_data<PointerDataType>(new_pointer);
		}
	}
}

bool Program::binary_func(std::function<Token(Token, Token)> func) {
	if (rel_token(tokens, 1).is_num_or_ptr() && rel_token(tokens, 2).is_num_or_ptr()) {
		shift_pointers(tokens, program_counter, -2);
		Token arg1 = rel_token(tokens, 1);
		Token arg2 = rel_token(tokens, 2);
		Token result = func(arg1, arg2);
		tokens.erase(tokens.begin() + program_counter);
		tokens.erase(tokens.begin() + program_counter);
		rel_token(tokens, 0) = result;
		return true;
	}
	return false;
}


