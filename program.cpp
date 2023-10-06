#include "program.h"

void throwUnexpectedCharException(char c, std::string current_word) {
	throw std::runtime_error("Current word: " + current_word + ", unexpected char: " + utils::char_to_str(c));
}

std::vector<Token> Program::tokenize(std::string str) {
	try {

		struct WordToken {
			std::string str;
			std::string display_str;
			ProgramCounterType line;
			WordToken(std::string str, ProgramCounterType line) {
				this->str = str;
				this->display_str = str;
				this->line = line;
			}
			WordToken(std::string str, std::string display_str, ProgramCounterType line) {
				this->str = str;
				this->display_str = display_str;
				this->line = line;
			}
		};

		struct Label {
			std::string str;
			PointerDataType token_index;
			Label(std::string str, PointerDataType token_index) {
				this->str = str;
				this->token_index = token_index;
			}
		};

		std::vector<WordToken> words;
		std::vector<Token> tokens;
		std::vector<Label> labels;
		if (str.size() < 1) {
			return tokens;
		}
		enum SplitterState {
			STATE_SPACE,
			STATE_WORD,
			STATE_STRING,
			STATE_ESCAPE,
			STATE_COMMENT,
		};
		str += EOF;
		SplitterState state = STATE_SPACE;
		std::string current_word = "";
		ProgramCounterType current_line = 1;
		try {
			for (ProgramCounterType i = 0; i < str.size(); i++) {
				char current_char = str[i];
				if (current_char < -1) {
					throw std::runtime_error("Invalid char: " + std::to_string(current_char));
				}
				if (state == STATE_WORD) {
					if (isspace(current_char)) {
						words.push_back(WordToken(current_word, current_line));
						current_word = "";
						state = STATE_SPACE;
					} else if (current_char == '#') {
						words.push_back(WordToken(current_word, current_line));
						current_word = "";
						state = STATE_COMMENT;
					} else if (current_char == EOF) {
						words.push_back(WordToken(current_word, current_line));
						break;
					} else {
						current_word += current_char;
					}
				} else if (state == STATE_SPACE) {
					if (isspace(current_char)) {
						// ok
					} else if (current_char == '"') {
						current_word = "";
						state = STATE_STRING;
					} else if (current_char == '#') {
						state = STATE_COMMENT;
					} else if (current_char == EOF) {
						break;
					} else {
						current_word = "";
						current_word += current_char;
						state = STATE_WORD;
					}
				} else if (state == STATE_STRING) {
					if (current_char == '"') {
						std::string repl_esc = utils::replace_escape_seq(current_word);
						repl_esc.insert(repl_esc.begin(), '"');
						repl_esc.insert(repl_esc.end(), '"');
						words.push_back(WordToken(repl_esc, current_line));
						state = STATE_SPACE;
					} else if (current_char == '\\') {
						state = STATE_ESCAPE;
					} else if (current_char == EOF) {
						throwUnexpectedCharException(current_char, current_word);
					} else {
						current_word += current_char;
					}
				} else if (state == STATE_ESCAPE) {
					if (current_char != '"') {
						current_word += '\\';
					}
					current_word += current_char;
					state = STATE_STRING;
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
		} catch (std::exception exc) {
			throw std::runtime_error("Line " + std::to_string(current_line) + ": " + std::string(exc.what()));
		}

		// replacing macros
		for (ProgramCounterType i = 0; i < words.size(); i++) {
			WordToken current_word_token = words[i];
			try {
				std::string current_word = current_word_token.str;
				if (current_word == "print") {
					words[i] = WordToken("sys", current_line);
					words.insert(words.begin() + i + 1, WordToken("0", current_line));
				}
			} catch (std::exception exc) {
				throw std::runtime_error("Line " + std::to_string(current_word_token.line) + ": " + std::string(exc.what()));
			}
		}

		// replacing string literals with lists
		for (ProgramCounterType i = 0; i < words.size(); i++) {
			WordToken current_word_token = words[i];
			try {
				std::string current_word = current_word_token.str;
				if (current_word.size() > 1 && current_word.front() == '"' && current_word.back() == '"') {
					std::string string_content = current_word.substr(1, current_word.size() - 2);
					std::string list_display_string = "list #\"" + utils::string_conv(string_content) + "\"";
					WordToken list_token = WordToken("list", list_display_string, current_word_token.line);
					words[i] = list_token;
					for (ProgramCounterType char_i = 0; char_i < string_content.size(); char_i++) {
						char c = string_content[char_i];
						std::string char_string = std::to_string(c);
						std::string char_display_string = char_string + " #'" + utils::char_to_str(c) + "'";
						WordToken char_token = WordToken(char_string, char_display_string, current_word_token.line);
						ProgramCounterType char_token_index = i + char_i + 1;
						words.insert(words.begin() + char_token_index, char_token);
					}
					WordToken end_token = WordToken("end", current_word_token.line);
					ProgramCounterType end_token_index = i + string_content.size() + 1;
					words.insert(words.begin() + end_token_index, end_token);
				}
			} catch (std::exception exc) {
				throw std::runtime_error("Line " + std::to_string(current_word_token.line) + ": " + std::string(exc.what()));
			}
		}

		// replacing type strings with type indices
		for (ProgramCounterType i = 0; i < words.size(); i++) {
			WordToken current_word_token = words[i];
			try {
				std::string current_word = current_word_token.str;
				token_type type = string_to_type(current_word);
				if (type != type_unknown) {
					int type_index = (int)type;
					words[i].str = std::to_string(type_index);
				}
			} catch (std::exception exc) {
				throw std::runtime_error("Line " + std::to_string(current_word_token.line) + ": " + std::string(exc.what()));
			}
		}

		// creating labels
		for (ProgramCounterType i = 0; i < words.size(); i++) {
			WordToken current_word_token = words[i];
			try {
				std::string current_word = current_word_token.str;
				if (current_word.front() == ':') {
					Label new_label(current_word.substr(1, current_word.size() - 1), i - 1);
					if (i < 1) {
						throw std::runtime_error("Label points at -1: " + current_word);
					}
					labels.push_back(new_label);
					words.erase(words.begin() + i);
					i--;
				}
			} catch (std::exception exc) {
				throw std::runtime_error("Line " + std::to_string(current_word_token.line) + ": " + std::string(exc.what()));
			}
		}

		// creating tokens
		for (ProgramCounterType i = 0; i < words.size(); i++) {
			WordToken current_word_token = words[i];
			try {
				std::string str = current_word_token.str;
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
				new_token.orig_str = current_word_token.display_str;
				tokens.push_back(new_token);
			} catch (std::exception exc) {
				throw std::runtime_error("Line " + std::to_string(current_word_token.line) + ": " + std::string(exc.what()));
			}
		}

		return tokens;

	} catch (std::exception exc) {
		throw std::runtime_error(__FUNCTION__": " + std::string(exc.what()));
	}
}

Node::Node(Token token) {
	this->token = token;
}

std::string Node::to_string() {
	//return token.to_string();
	return token.orig_str;
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
	try {
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
			local_print_buffer = "";
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
				} else if (current_token_read.str == "log") {
					if (unary_func([](Token a) { return Token::log(a); })) {
						break;
					}
				} else if (current_token_read.str == "log2") {
					if (unary_func([](Token a) { return Token::log2(a); })) {
						break;
					}
				} else if (current_token_read.str == "sin") {
					if (unary_func([](Token a) { return Token::sin(a); })) {
						break;
					}
				} else if (current_token_read.str == "cos") {
					if (unary_func([](Token a) { return Token::cos(a); })) {
						break;
					}
				} else if (current_token_read.str == "tan") {
					if (unary_func([](Token a) { return Token::tan(a); })) {
						break;
					}
				} else if (current_token_read.str == "asin") {
					if (unary_func([](Token a) { return Token::asin(a); })) {
						break;
					}
				} else if (current_token_read.str == "acos") {
					if (unary_func([](Token a) { return Token::acos(a); })) {
						break;
					}
				} else if (current_token_read.str == "atan") {
					if (unary_func([](Token a) { return Token::atan(a); })) {
						break;
					}
				} else if (current_token_read.str == "atan2") {
					if (binary_func([](Token a, Token b) { return Token::atan2(a, b); })) {
						break;
					}
				} else if (current_token_read.str == "floor") {
					if (unary_func([](Token a) { return Token::floor(a); })) {
						break;
					}
				} else if (current_token_read.str == "ceil") {
					if (unary_func([](Token a) { return Token::ceil(a); })) {
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
				} else if (current_token_read.str == "and") {
					if (binary_func([](Token a, Token b) { return Token::and_op(a, b); })) {
						break;
					}
				} else if (current_token_read.str == "or") {
					if (binary_func([](Token a, Token b) { return Token::or_op(a, b); })) {
						break;
					}
				} else if (current_token_read.str == "xor") {
					if (binary_func([](Token a, Token b) { return Token::xor_op(a, b); })) {
						break;
					}
				} else if (current_token_read.str == "not") {
					if (unary_func([](Token a) { return Token::not_op(a); })) {
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
				} else if (current_token_read.str == "cast") {
					if (rel_token(tokens, 1).is_num_or_ptr() && rel_token(tokens, 2).is_num_or_ptr()) {
						shift_pointers(tokens, program_counter, -2);
						Token arg1 = rel_token(tokens, 1);
						Token arg2 = rel_token(tokens, 2);
						token_type type = static_cast<token_type>(arg1.get_data_cast<int>());
						if (type >= 0 && type < type_unknown) {
							if (type == type_instr) {
								int instruction_index = arg2.get_data_cast<int>();
								if (get_instruction_info(instruction_index).index != -1) {
									arg2.cast(type);
								}
							} else {
								arg2.cast(type);
							}
						}
						Token result = arg2;
						result.str = result.to_string();
						tokens.erase(tokens.begin() + program_counter);
						tokens.erase(tokens.begin() + program_counter);
						rel_token(tokens, 0) = result;
						break;
					}
				} else if (current_token_read.str == "sys") {
					bool arg1_valid = rel_token(tokens, 1).is_num_or_ptr();
					bool arg2_valid = rel_token(tokens, 2).is_list_header() || rel_token(tokens, 2).is_singular_data();
					if (arg1_valid && arg2_valid) {
						ProgramCounterType sys_position = program_counter;
						int sys_call_index = rel_token(tokens, 1).get_data_cast<int>();
						std::vector<Token> arg_list;
						if (rel_token(tokens, 2).is_singular_data()) {
							arg_list.push_back(rel_token(tokens, 2));
							shift_pointers(tokens, sys_position, -3);
							tokens.erase(tokens.begin() + sys_position, tokens.begin() + sys_position + 3);
						} else {
							ProgramCounterType current_token_offset = 3;
							while (!rel_token(tokens, current_token_offset).is_list_end()) {
								Token arg_token = rel_token(tokens, current_token_offset);
								arg_list.push_back(arg_token);
								current_token_offset++;
							}
							ProgramCounterType sys_tokens_size = current_token_offset - sys_position + 1;
							shift_pointers(tokens, sys_position, -(PointerDataType)sys_tokens_size);
							tokens.erase(tokens.begin() + sys_position, tokens.begin() + sys_position + current_token_offset + 1);
						}
						sys_call(sys_call_index, arg_list);
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
			if (print_buffer_enabled && local_print_buffer.size() > 0) {
				if (print_iterations) {
					std::cout << "Print: ";
				}
				std::cout << local_print_buffer;
				if (print_iterations && !utils::is_newline(local_print_buffer.back())) {
					std::cout << "\n";
				}
				global_print_buffer += local_print_buffer;
			}
			if (tokens == prev_tokens) {
				break;
			}
			prev_tokens = tokens;
		}
		return tokens;
	} catch (std::exception exc) {
		throw std::runtime_error(__FUNCTION__": " + std::string(exc.what()));
	}
}

void Program::parse() {
	try {
		nodes.clear();
		Node* parent_node;
		for (PointerDataType token_i = 0; token_i < tokens.size(); token_i++) {
			PointerDataType new_token_i;
			std::unique_ptr<Node> node = parse_token(tokens, token_i, nullptr, new_token_i);
			nodes.push_back(std::move(node));
			token_i = new_token_i;
		}
	} catch (std::exception exc) {
		throw std::runtime_error("Node parse error: " + std::string(exc.what()));
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

std::unique_ptr<Node> Program::parse_token(
	std::vector<Token>& token_list, PointerDataType parse_token_index,
	Node* parent_node, PointerDataType& new_token_index, int depth
) {
	if (depth >= MAX_NESTED_NODES) {
		throw std::runtime_error("Too many nested nodes: " + std::to_string(depth));
	}
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
				std::unique_ptr<Node> node = parse_token(
					token_list,
					token_index(token_list, cur_index), new_node_p, parse_token_index, depth + 1
				);
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
				std::unique_ptr<Node> node = parse_token(
					token_list, parse_token_index + 1, new_node_p, parse_token_index
				);
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

bool Program::unary_func(std::function<Token(Token)> func) {
	if (rel_token(tokens, 1).is_num_or_ptr()) {
		shift_pointers(tokens, program_counter, -1);
		Token arg = rel_token(tokens, 1);
		Token result = func(arg);
		tokens.erase(tokens.begin() + program_counter);
		rel_token(tokens, 0) = result;
		return true;
	}
	return false;
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

std::vector<Token> Program::sys_call(int index, std::vector<Token> input) {
	switch (index) {
		case 0: return sys_println(input); break;
		default: return std::vector<Token>();
	}
}

std::vector<Token> Program::sys_println(std::vector<Token> input) {
	std::string str;
	for (Token token : input) {
		char c = token.get_data_cast<int>();
		str += c;
	}
	local_print_buffer += str;
	return std::vector<Token>();
}
