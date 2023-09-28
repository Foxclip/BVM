#include "algorithm.h"

void throwUnexpectedCharException(char c, std::string current_word) {
	throw std::runtime_error("Current word: " + current_word + ", unexpected char: " + std::string(1, c));
}

Token::Token() {}

Token::Token(std::string str, token_type type) {
	this->str = str;
	this->type = type;
}

bool Token::is_num() {
	switch (type) {
		case type_int:
		case type_long:
		case type_uint:
		case type_ulong:
		case type_float:
		case type_double:
			return true;
		default:
			return false;
	}
}

bool Token::is_ptr() {
	return type == type_ptr;
}

bool Token::is_num_or_ptr() {
	return is_num() || is_ptr();
}

std::string Token::to_string() const {
	switch (type) {
		case type_int:
			return std::to_string(data.m_int) + "i";
		case type_long:
			return std::to_string(data.m_long) + "L";
		case type_uint:
			return std::to_string(data.m_uint) + "ui";
		case type_ulong:
			return std::to_string(data.m_ulong) + "UL";
		case type_float:
			return std::to_string(data.m_float) + "f";
		case type_double:
			return std::to_string(data.m_double) + "d";
		case type_instr:
			return INSTRUCTION_LIST[get_data<InstructionDataType>()].str;
		case type_ptr:
			return "p(" + std::to_string(get_data<PointerDataType>()) + ")";
		default:
			throw std::runtime_error("Unknown token_data type: " + std::to_string(type));
	}
}

bool operator==(const Token& first, const Token& second) {
	if (first.type == second.type) {
		switch (first.type) {
			case type_int:
				return first.data.m_int == second.data.m_int;
			case type_long:
				return first.data.m_long == second.data.m_long;
			case type_uint:
				return first.data.m_uint == second.data.m_uint;
			case type_ulong:
				return first.data.m_ulong == second.data.m_ulong;
			case type_float:
				return first.data.m_float == second.data.m_float;
			case type_double:
				return first.data.m_double == second.data.m_double;
			case type_instr:
				return first.get_data<InstructionDataType>() == second.get_data<InstructionDataType>();
			case type_ptr:
				return first.get_data<PointerDataType>() == second.get_data<PointerDataType>();
			default:
				throw std::runtime_error("Unknown token_data type: " + std::to_string(first.type));
		}
	}
	return false;
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
			if (utils::is_valid_word_middle(current_char)) {
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
			} else if (utils::is_valid_word_prefix(current_char)) {
				current_word = "";
				current_word += current_char;
				state = WORD;
			} else if (utils::is_number_prefix(current_char)) {
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
		Token new_token;
		if (utils::is_number(str)) {
			new_token = Token(str, get_token_type<DefaultNumType>());
			// TODO: parse float and double
			new_token.set_data<long>(std::stol(str));
		} else {
			auto it = std::find_if(labels.begin(), labels.end(),
				[&](Label& label) {
					return label.str == str;
				}
			);
			if (it != labels.end()) {
				PointerDataType relative_address = (*it).token_index - i;
				new_token = Token(str, get_token_type<PointerTokenType>());
				new_token.set_data<PointerDataType>(relative_address);
			} else {
				new_token = Token(str, get_token_type<InstructionTokenType>());
				new_token.set_data<InstructionDataType>(get_instruction_info(str).index);
			}
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
			Token current_token_read = rel_token(tokens, 0);
			Token next_token = rel_token(tokens, 1);
#define BINARY_FUNC(TRet, FN) if (binary_func<TRet>([](TRet a, TRet b) { FN })) { break; }
#define CALL_BINARY_FUNC(Macro_func) \
			token_type type1 = rel_token(tokens, 1).type; \
			token_type type2 = rel_token(tokens, 2).type; \
			if (rel_token(tokens, 1).is_num_or_ptr() && rel_token(tokens, 2).is_num_or_ptr()) { \
				token_type return_type = get_return_type(type1, type2); \
				if (return_type == type_double) { \
					Macro_func(double) \
				} else if (return_type == type_float) { \
					Macro_func(float) \
				} else if (return_type == type_ptr) { \
					Macro_func(PointerDataType) \
				} else if (return_type == type_ulong) { \
					Macro_func(unsigned long) \
				} else if (return_type == type_long) { \
					Macro_func(long) \
				} else if (return_type == type_uint) { \
					Macro_func(unsigned int) \
				} else if (return_type == type_int) { \
					Macro_func(int) \
				} \
			}
			if (current_token_read.is_num_or_ptr()) {
				// skipping
			} else if (current_token_read.str == "add") {
#define ADD_FUNC(TRet) BINARY_FUNC(TRet, return a + b;)
				CALL_BINARY_FUNC(ADD_FUNC)
			} else if (current_token_read.str == "sub") {
#define SUB_FUNC(TRet) BINARY_FUNC(TRet, return a - b;)
				CALL_BINARY_FUNC(SUB_FUNC);
			} else if (current_token_read.str == "mul") {
#define MUL_FUNC(TRet) BINARY_FUNC(TRet, return a * b;)
				CALL_BINARY_FUNC(MUL_FUNC);
			} else if (current_token_read.str == "div") {
#define DIV_FUNC(TRet) BINARY_FUNC(TRet, return a / b;)
				CALL_BINARY_FUNC(DIV_FUNC);
			} else if (current_token_read.str == "mod") {
#define MOD_FUNC(TRet) BINARY_FUNC(TRet, return utils::mod(a, b);)
				token_type type1 = rel_token(tokens, 1).type;
				token_type type2 = rel_token(tokens, 2).type;
				token_type return_type = get_return_type(type1, type2);
				if (return_type == type_double) {
					MOD_FUNC(long)
				} else if (return_type == type_float) {
					MOD_FUNC(long)
				} else if (return_type == type_ptr) {
					MOD_FUNC(PointerDataType)
				} else if (return_type == type_ulong) {
					MOD_FUNC(unsigned long)
				} else if (return_type == type_long) {
					MOD_FUNC(long)
				} else if (return_type == type_uint) {
					MOD_FUNC(unsigned int)
				} else if (return_type == type_int) {
					MOD_FUNC(int)
				}
			} else if (current_token_read.str == "pow") {
#define POW_FUNC(TRet) BINARY_FUNC(TRet, return pow(a, b);)
				CALL_BINARY_FUNC(POW_FUNC);
			} else if (current_token_read.str == "cmp") {
#define CMP_FUNC(TRet) BINARY_FUNC(TRet, return a == b;)
				CALL_BINARY_FUNC(CMP_FUNC);
			} else if (current_token_read.str == "lt") {
#define LT_FUNC(TRet) BINARY_FUNC(TRet, return a < b;)
				CALL_BINARY_FUNC(LT_FUNC);
			} else if (current_token_read.str == "gt") {
#define GT_FUNC(TRet) BINARY_FUNC(TRet, return a > b;)
				CALL_BINARY_FUNC(GT_FUNC);
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
			} else if (current_token_read.str == "repl") {
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
			} else if (current_token_read.str == "replp") {
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
					int cond = rel_token(tokens, 1).get_data_cast<int>();
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
	std::vector<long> results;
	for (int i = 0; i < tokens.size(); i++) {
		long result = tokens[i].get_data_cast<long>();
		results.push_back(result);
	}
	return results;
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
	for (PointerDataType token_i = 0; token_i < token_list.size(); token_i++) {
		Token current_token = get_token(token_list, token_i);
		if (current_token.is_ptr()) {
			PointerDataType pointer_index_old = token_i;
			PointerDataType pointer_dst_old = token_i + current_token.get_data_cast<PointerDataType>();
			PointerDataType pointer_index_new = pointer_index_old;
			PointerDataType pointer_dst_new = pointer_dst_old;
			if (offset < 0) {
				PointerDataType recalc_offset = std::clamp(pos - pointer_index_old, offset, 0L);
				if (pos <= pointer_index_old) {
					pointer_index_new += recalc_offset;
				}
			} else {
				if (pos <= pointer_index_old) {
					pointer_index_new += offset;
				}
			}
			if (offset < 0) {
				PointerDataType recalc_offset = std::clamp(pos - pointer_dst_old, offset, 0L);
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

InstructionInfo Program::get_instruction_info(std::string token) {
	auto it = std::find_if(INSTRUCTION_LIST.begin(), INSTRUCTION_LIST.end(),
		[&](InstructionDef def) {
			return def.str == token;
		}
	);
	if (it == INSTRUCTION_LIST.end()) {
		throw std::runtime_error("Instruction not found: " + token);
	}
	const InstructionDef def = *it;
	int index = it - INSTRUCTION_LIST.begin();
	InstructionInfo info(def.str, def.arg_count, index);
	return info;
}

token_type Program::get_return_type(token_type type1, token_type type2) {
	if (type1 == type_double || type2 == type_double) {
		return type_double;
	} else if (type1 == type_float || type2 == type_float) {
		return type_float;
	} else if (type1 == type_ptr || type2 == type_ptr) {
		return type_ptr;
	} else if (type1 == type_ulong || type2 == type_ulong) {
		return type_ulong;
	} else if (type1 == type_long || type2 == type_long) {
		return type_long;
	} else if (type1 == type_uint || type2 == type_uint) {
		return type_uint;
	} else if (type1 == type_int || type2 == type_int) {
		return type_int;
	} else {
		throw std::runtime_error("Cannot determine return type: " + std::to_string(type1) + " " + std::to_string(type2));
	}
}


