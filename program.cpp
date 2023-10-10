#include "program.h"

Program::DeleteOp::DeleteOp(ProgramCounterType pos_begin, ProgramCounterType pos_end) {
	this->pos_begin = pos_begin;
	this->pos_end = pos_end;
}

Program::InsertOp::InsertOp(ProgramCounterType new_pos, std::vector<Token> insert_tokens) {
	this->new_pos = new_pos;
	this->insert_tokens = insert_tokens;
	this->recalc_pointers = false;
}

Program::InsertOp::InsertOp(ProgramCounterType old_pos, ProgramCounterType new_pos, std::vector<Token> insert_tokens) {
	this->old_pos = old_pos;
	this->new_pos = new_pos;
	this->insert_tokens = insert_tokens;
	this->recalc_pointers = true;
}

Program::ModifyOp::ModifyOp(ProgramCounterType pos, Token new_token) {
	this->new_pos = pos;
	this->new_token = new_token;
	this->recalc_pointers = false;
}

Program::ModifyOp::ModifyOp(ProgramCounterType old_pos, ProgramCounterType new_pos, Token new_token) {
	this->old_pos = old_pos;
	this->new_pos = new_pos;
	this->new_token = new_token;
	this->recalc_pointers = true;
}

Program::ReplaceOp::ReplaceOp(
	ProgramCounterType dst_begin, ProgramCounterType dst_end,
	ProgramCounterType src_begin, std::vector<Token> src_tokens
) {
	this->dst_begin = dst_begin;
	this->dst_end = dst_end;
	this->src_begin = src_begin;
	this->src_tokens = src_tokens;
}

Program::MoveOp::MoveOp(
	ProgramCounterType old_begin, ProgramCounterType old_end,
	ProgramCounterType new_begin
) {
	this->old_begin = old_begin;
	this->old_end = old_end;
	this->new_begin = new_begin;
}

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
					new_token.str = new_token.to_string();
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

void Program::print_tokens(std::vector<Token>& token_list, bool print_program_counter) {
	for (ProgramCounterType i = 0; i < token_list.size(); i++) {
		if (print_program_counter && i == program_counter) {
			std::cout << "*";
		}
		std::cout << token_list[i].to_string() << " ";
	}
	if (print_program_counter && program_counter == token_list.size()) {
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
			print_tokens(tokens, false);
		}
		for (ProgramCounterType iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
			if (print_iterations) {
				std::cout << "Iteration " << iteration << ": ";
			}
			ProgramCounterType steps = 0;
			std::stack<ProgramCounterType> list_scope_stack;
			reset_index_shift();
			modify_ops.clear();
			delete_ops.clear();
			insert_ops.clear();
			replace_ops.clear();
			move_ops.clear();
			local_print_buffer = "";
			for (program_counter = 0; program_counter < prev_tokens.size(); program_counter++) {
				Token current_token = rel_token(prev_tokens, 0);
				if (current_token.is_num_or_ptr()) {
					// skipping
				} else if (current_token.str == "add") {
					binary_func([](Token a, Token b) { return Token::add(a, b); });
				} else if (current_token.str == "sub") {
					binary_func([](Token a, Token b) { return Token::sub(a, b); });
				} else if (current_token.str == "mul") {
					binary_func([](Token a, Token b) { return Token::mul(a, b); });
				} else if (current_token.str == "div") {
					binary_func([](Token a, Token b) { return Token::div(a, b); });
				} else if (current_token.str == "mod") {
					binary_func([](Token a, Token b) { return Token::mod(a, b); });
				} else if (current_token.str == "pow") {
					binary_func([](Token a, Token b) { return Token::pow(a, b); });
				} else if (current_token.str == "log") {
					unary_func([](Token a) { return Token::log(a); });
				} else if (current_token.str == "log2") {
					unary_func([](Token a) { return Token::log2(a); });
				} else if (current_token.str == "sin") {
					unary_func([](Token a) { return Token::sin(a); });
				} else if (current_token.str == "cos") {
					unary_func([](Token a) { return Token::cos(a); });
				} else if (current_token.str == "tan") {
					unary_func([](Token a) { return Token::tan(a); });
				} else if (current_token.str == "asin") {
					unary_func([](Token a) { return Token::asin(a); });
				} else if (current_token.str == "acos") {
					unary_func([](Token a) { return Token::acos(a); });
				} else if (current_token.str == "atan") {
					unary_func([](Token a) { return Token::atan(a); });
				} else if (current_token.str == "atan2") {
					binary_func([](Token a, Token b) { return Token::atan2(a, b); });
				} else if (current_token.str == "floor") {
					unary_func([](Token a) { return Token::floor(a); });
				} else if (current_token.str == "ceil") {
					unary_func([](Token a) { return Token::ceil(a); });
				} else if (current_token.str == "cmp") {
					binary_func([](Token a, Token b) { return Token::cmp(a, b); });
				} else if (current_token.str == "lt") {
					binary_func([](Token a, Token b) { return Token::lt(a, b); });
				} else if (current_token.str == "gt") {
					binary_func([](Token a, Token b) { return Token::gt(a, b); });
				} else if (current_token.str == "and") {
					binary_func([](Token a, Token b) { return Token::and_op(a, b); });
				} else if (current_token.str == "or") {
					binary_func([](Token a, Token b) { return Token::or_op(a, b); });
				} else if (current_token.str == "xor") {
					binary_func([](Token a, Token b) { return Token::xor_op(a, b); });
				} else if (current_token.str == "not") {
					unary_func([](Token a) { return Token::not_op(a); });
				} else if (current_token.str == "cpy") {
					if (rel_token(prev_tokens, 1).is_num_or_ptr() && rel_token(prev_tokens, 2).is_num_or_ptr()) {
						PointerDataType src = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
						PointerDataType dst = rel_token(prev_tokens, 2).get_data_cast<PointerDataType>();
						PointerDataType new_token_index;
						ProgramCounterType src_index_begin = token_index(prev_tokens, program_counter + 1 + src);
						ProgramCounterType dst_index = token_index(prev_tokens, program_counter + 2 + dst);
						std::unique_ptr<Node> node = parse_token(prev_tokens, token_index(prev_tokens, src_index_begin), nullptr, new_token_index);
						std::vector<Token> node_tokens = node.get()->tokenize();
						delete_tokens(program_counter, program_counter + 3);
						insert_tokens(src_index_begin, dst_index, node_tokens);
					}
				} else if (current_token.str == "del") {
					if (rel_token(prev_tokens, 1).is_num_or_ptr()) {
						PointerDataType arg = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
						PointerDataType new_token_index;
						ProgramCounterType target_index = token_index(prev_tokens, program_counter + 1 + arg);
						std::unique_ptr<Node> node = parse_token(prev_tokens, token_index(prev_tokens, target_index), nullptr, new_token_index);
						std::vector<Token> node_tokens = node.get()->tokenize();
						delete_tokens(program_counter, program_counter + 2);
						delete_tokens(target_index, target_index + node_tokens.size());
					}
				} else if (current_token.str == "get") {
					if (rel_token(prev_tokens, 1).is_num_or_ptr()) {
						PointerDataType src = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
						PointerDataType src_index_begin = token_index(prev_tokens, program_counter + 1 + src);
						PointerDataType src_last_index;
						std::unique_ptr<Node> src_node = parse_token(prev_tokens, token_index(prev_tokens, src_index_begin), nullptr, src_last_index);
						std::vector<Token> src_node_tokens = src_node.get()->tokenize();
						replace_tokens(program_counter, program_counter + 2, src_index_begin, src_node_tokens);
						break;
					}
				//} else if (current_token.str == "set") {
				//	if (rel_token(tokens, 1).is_num_or_ptr()) {
				//		PointerDataType dst = rel_token(tokens, 1).get_data_cast<PointerDataType>();
				//		PointerDataType dst_index_begin = token_index(tokens, program_counter + 1 + dst);
				//		PointerDataType src_index_begin = token_index(tokens, program_counter + 2);
				//		PointerDataType dst_last_index;
				//		PointerDataType repl_index = program_counter;
				//		PointerDataType repl_last_index;
				//		std::unique_ptr<Node> repl_node = parse_token(tokens, repl_index, nullptr, repl_last_index);
				//		if (dst_index_begin >= repl_index && dst_index_begin <= repl_last_index) {
				//			dst_index_begin = repl_index;
				//		}
				//		Node* src_node = repl_node.get()->arguments[1].get();
				//		std::vector<Token> repl_node_tokens = repl_node->tokenize();
				//		std::vector<Token> src_node_tokens = src_node->tokenize();
				//		std::unique_ptr<Node> dst_node = parse_token(tokens, token_index(tokens, dst_index_begin), nullptr, dst_last_index);
				//		std::vector<Token> dst_node_tokens = dst_node.get()->tokenize();
				//		PointerDataType insertion_index = dst_index_begin;
				//		if (dst_index_begin != repl_index) {
				//			shift_pointers(tokens, repl_index, -(PointerDataType)repl_node_tokens.size());
				//			tokens.erase(tokens.begin() + repl_index, tokens.begin() + repl_last_index + 1);
				//			if (insertion_index > repl_index) {
				//				insertion_index -= repl_node_tokens.size();
				//			}
				//		}
				//		PointerDataType pointer_offset = src_node_tokens.size() - dst_node_tokens.size();
				//		shift_pointers(tokens, insertion_index, pointer_offset);
				//		tokens.erase(tokens.begin() + insertion_index, tokens.begin() + insertion_index + dst_node_tokens.size());
				//		tokens.insert(tokens.begin() + insertion_index, src_node_tokens.begin(), src_node_tokens.end());
				//		break;
				//	}
				//} else if (current_token.str == "repl") {
				//	if (rel_token(tokens, 1).is_num_or_ptr() && rel_token(tokens, 2).is_num_or_ptr()) {
				//		PointerDataType dst = rel_token(tokens, 1).get_data_cast<PointerDataType>();
				//		PointerDataType src = rel_token(tokens, 2).get_data_cast<PointerDataType>();
				//		PointerDataType dst_index_begin = token_index(tokens, program_counter + 1 + dst);
				//		PointerDataType src_index_begin = token_index(tokens, program_counter + 2 + src);
				//		PointerDataType src_last_index;
				//		PointerDataType dst_last_index;
				//		PointerDataType repl_index = program_counter;
				//		if (dst_index_begin >= repl_index && dst_index_begin < repl_index + 3) {
				//			dst_index_begin = repl_index;
				//		}
				//		std::unique_ptr<Node> src_node = parse_token(tokens, token_index(tokens, src_index_begin), nullptr, src_last_index);
				//		std::vector<Token> src_node_tokens = src_node.get()->tokenize();
				//		std::unique_ptr<Node> dst_node = parse_token(tokens, token_index(tokens, dst_index_begin), nullptr, dst_last_index);
				//		std::vector<Token> dst_node_tokens = dst_node.get()->tokenize();
				//		PointerDataType insertion_index = dst_index_begin;
				//		if (dst_index_begin != repl_index) {
				//			shift_pointers(tokens, repl_index, -3);
				//			tokens.erase(tokens.begin() + repl_index);
				//			tokens.erase(tokens.begin() + repl_index);
				//			tokens.erase(tokens.begin() + repl_index);
				//			if (insertion_index > repl_index) {
				//				insertion_index -= 3;
				//			}
				//		}
				//		PointerDataType pointer_offset = src_node_tokens.size() - dst_node_tokens.size();
				//		shift_pointers(tokens, insertion_index, pointer_offset);
				//		tokens.erase(tokens.begin() + insertion_index, tokens.begin() + insertion_index + dst_node_tokens.size());
				//		tokens.insert(tokens.begin() + insertion_index, src_node_tokens.begin(), src_node_tokens.end());
				//		break;
				//	}
				//} else if (current_token.str == "ins") {
				//	if (rel_token(tokens, 1).is_num_or_ptr()) {
				//		PointerDataType dst = rel_token(tokens, 1).get_data_cast<PointerDataType>();
				//		PointerDataType dst_index = token_index(tokens, program_counter + 1 + dst);
				//		PointerDataType src_index_begin = token_index(tokens, program_counter + 2);
				//		PointerDataType ins_index = program_counter;
				//		PointerDataType ins_last_index;
				//		std::unique_ptr<Node> ins_node = parse_token(tokens, ins_index, nullptr, ins_last_index);
				//		std::vector<Token> ins_node_tokens = ins_node->tokenize();
				//		if (dst_index >= ins_index && dst_index <= ins_last_index) {
				//			dst_index = ins_index;
				//		}
				//		Node* src_node = ins_node.get()->arguments[1].get();
				//		std::vector<Token> src_node_tokens = src_node->tokenize();
				//		PointerDataType insertion_index = dst_index;
				//		shift_pointers(tokens, ins_index, -(PointerDataType)ins_node_tokens.size());
				//		tokens.erase(tokens.begin() + ins_index, tokens.begin() + ins_last_index + 1);
				//		if (insertion_index > ins_index) {
				//			insertion_index -= std::min(insertion_index - ins_index, (PointerDataType)ins_node_tokens.size());
				//		}
				//		PointerDataType pointer_offset = src_node_tokens.size();
				//		shift_pointers(tokens, insertion_index, pointer_offset);
				//		tokens.insert(tokens.begin() + insertion_index, src_node_tokens.begin(), src_node_tokens.end());
				//		break;
				//	}
				//} else if (current_token.str == "if") {
				//	if (rel_token(tokens, 1).is_num_or_ptr()) {
				//		BoolType cond = rel_token(tokens, 1).get_data_cast<BoolType>();
				//		PointerDataType new_token_index;
				//		std::unique_ptr<Node> if_node = parse_token(tokens, token_index(tokens, program_counter), nullptr, new_token_index);
				//		std::vector<Token> if_node_tokens = if_node.get()->tokenize();
				//		Node* true_node = if_node.get()->arguments[1].get();
				//		Node* false_node = if_node.get()->arguments[2].get();
				//		std::vector<Token> true_node_tokens = true_node->tokenize();
				//		std::vector<Token> false_node_tokens = false_node->tokenize();
				//		PointerDataType true_node_offset = 2;
				//		PointerDataType false_node_offset = 2 + true_node_tokens.size();
				//		PointerDataType true_node_index = program_counter + true_node_offset;
				//		PointerDataType false_node_index = program_counter + false_node_offset;
				//		if (cond != 0) {
				//			shift_pointers(tokens, false_node_index, -(PointerDataType)false_node_tokens.size());
				//			tokens.erase(tokens.begin() + false_node_index, tokens.begin() + false_node_index + false_node_tokens.size());
				//			shift_pointers(tokens, program_counter, -true_node_offset);
				//			tokens.erase(tokens.begin() + program_counter);
				//			tokens.erase(tokens.begin() + program_counter);
				//		} else {
				//			shift_pointers(tokens, program_counter, -false_node_offset);
				//			tokens.erase(tokens.begin() + program_counter, tokens.begin() + false_node_index);
				//		}
				//		break;
				//	}
				} else if (current_token.str == "list") {
					list_scope_stack.push(program_counter);
				} else if (current_token.str == "end") {
					ProgramCounterType list_pos = list_scope_stack.top();
					delete_tokens(list_pos, list_pos + 1);
					delete_tokens(program_counter, program_counter + 1);
					list_scope_stack.pop();
				//} else if (current_token.str == "cast") {
				//	if (rel_token(tokens, 1).is_num_or_ptr() && rel_token(tokens, 2).is_num_or_ptr()) {
				//		shift_pointers(tokens, program_counter, -2);
				//		Token arg1 = rel_token(tokens, 1);
				//		Token arg2 = rel_token(tokens, 2);
				//		token_type type = static_cast<token_type>(arg1.get_data_cast<int>());
				//		if (type >= 0 && type < type_unknown) {
				//			if (type == type_instr) {
				//				int instruction_index = arg2.get_data_cast<int>();
				//				if (get_instruction_info(instruction_index).index != -1) {
				//					arg2.cast(type);
				//				}
				//			} else {
				//				arg2.cast(type);
				//			}
				//		}
				//		Token result = arg2;
				//		result.str = result.to_string();
				//		tokens.erase(tokens.begin() + program_counter);
				//		tokens.erase(tokens.begin() + program_counter);
				//		rel_token(tokens, 0) = result;
				//		break;
				//	}
				//} else if (current_token.str == "sys") {
				//	bool arg1_valid = rel_token(tokens, 1).is_num_or_ptr();
				//	bool arg2_valid = rel_token(tokens, 2).is_list_header() || rel_token(tokens, 2).is_singular_data();
				//	if (arg1_valid && arg2_valid) {
				//		ProgramCounterType sys_position = program_counter;
				//		int sys_call_index = rel_token(tokens, 1).get_data_cast<int>();
				//		std::vector<Token> arg_list;
				//		if (rel_token(tokens, 2).is_singular_data()) {
				//			arg_list.push_back(rel_token(tokens, 2));
				//			shift_pointers(tokens, sys_position, -3);
				//			tokens.erase(tokens.begin() + sys_position, tokens.begin() + sys_position + 3);
				//		} else {
				//			ProgramCounterType current_token_offset = 3;
				//			while (!rel_token(tokens, current_token_offset).is_list_end()) {
				//				Token arg_token = rel_token(tokens, current_token_offset);
				//				arg_list.push_back(arg_token);
				//				current_token_offset++;
				//			}
				//			ProgramCounterType sys_tokens_size = current_token_offset - sys_position + 1;
				//			shift_pointers(tokens, sys_position, -(PointerDataType)sys_tokens_size);
				//			tokens.erase(tokens.begin() + sys_position, tokens.begin() + sys_position + current_token_offset + 1);
				//		}
				//		sys_call(sys_call_index, arg_list);
				//		break;
				//	}
				} else {
					throw std::runtime_error("Unexpected token: " + current_token.str);
				}
				steps++;
			}
			exec_pending_ops();
			if (print_iterations) {
				print_tokens(tokens, false);
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
	return utils::mod(index, (PointerDataType)token_list.size() + 1);
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

PointerDataType Program::to_dst_index(ProgramCounterType old_index) {
	return index_shift[old_index];
}

void Program::shift_indices(ProgramCounterType pos, PointerDataType offset) {
	for (ProgramCounterType index = pos; index < index_shift.size(); index++) {
		if (offset < 0) {
			if (index < pos - offset) {
				index_shift[index] = -1;
			} else {
				if (index_shift[index] > 0) {
					index_shift[index] += offset;
				}
			}
		} else {
			if (index_shift[index] > 0) {
				index_shift[index] += offset;
			}
		}
	}
}

void Program::reset_index_shift() {
	index_shift = std::vector<PointerDataType>(prev_tokens.size() + 1);
	for (ProgramCounterType index = 0; index < index_shift.size(); index++) {
		index_shift[index] = index;
	}
}

void Program::delete_tokens(ProgramCounterType pos_begin, ProgramCounterType pos_end) {
	delete_ops.push_back(DeleteOp(pos_begin, pos_end));
}

void Program::insert_tokens(ProgramCounterType old_pos, ProgramCounterType new_pos, std::vector<Token> insert_tokens) {
	insert_ops.push_back(InsertOp(old_pos, new_pos, insert_tokens));
}

void Program::modify_token(ProgramCounterType pos, Token new_token) {
	modify_ops.push_back(ModifyOp(pos, new_token));
}

void Program::replace_tokens(
	ProgramCounterType dst_begin, ProgramCounterType dst_end,
	ProgramCounterType src_begin, std::vector<Token> src_tokens
) {
	replace_ops.push_back(ReplaceOp(dst_begin, dst_end, src_begin, src_tokens));
}

ProgramCounterType Program::calc_delete_offset(ProgramCounterType pos_begin, ProgramCounterType pos_end) {
	ProgramCounterType valid_token_count = 0;
	PointerDataType dst_erase_begin = to_dst_index(pos_begin);
	for (ProgramCounterType src_index = pos_begin; src_index < pos_end; src_index++) {
		PointerDataType dst_index = to_dst_index(src_index);
		if (dst_index >= 0) {
			valid_token_count++;
		}
	}
	PointerDataType offset = -(PointerDataType)valid_token_count;
	return offset;
}

void Program::delete_op_exec(
	ProgramCounterType pos_begin, ProgramCounterType pos_end
) {
	PointerDataType dst_erase_begin = to_dst_index(pos_begin);
	PointerDataType offset = calc_delete_offset(pos_begin, pos_end);
	for (ProgramCounterType src_erase_index = pos_begin; src_erase_index < pos_end; src_erase_index++) {
		PointerDataType dst_erase_index = to_dst_index(src_erase_index);
		if (dst_erase_index >= 0) {
			tokens.erase(tokens.begin() + dst_erase_begin);
		}
	}
	shift_indices(pos_begin, offset);
}

void Program::insert_op_exec(
	ProgramCounterType old_pos, ProgramCounterType new_pos,
	std::vector<Token> insert_tokens,
	OpType op_type
) {
	PointerDataType dst_insert_index = -1;
	for (ProgramCounterType token_i = new_pos; ; token_i++) {
		PointerDataType dst_index = to_dst_index(token_i);
		if (dst_index >= 0) {
			dst_insert_index = dst_index;
			break;
		}
	}
	PointerDataType offset = insert_tokens.size();
	if (op_type == OP_TYPE_NORMAL) {
		shift_pointers(tokens, dst_insert_index, offset);
	}
	shift_indices(new_pos, offset);
	tokens.insert(tokens.begin() + dst_insert_index, insert_tokens.begin(), insert_tokens.end());
	for (ProgramCounterType token_i = 0; token_i < insert_tokens.size(); token_i++) {
		Token current_token = insert_tokens[token_i];
		if (current_token.is_ptr()) {
			PointerDataType pointer_index_old = old_pos + token_i;
			PointerDataType pointer_old = current_token.get_data_cast<PointerDataType>();
			PointerDataType pointer_dst_old = token_index(prev_tokens, pointer_index_old + pointer_old);
			PointerDataType pointer_index_new = dst_insert_index + token_i;
			PointerDataType pointer_dst_new;
			if (pointer_dst_old >= old_pos && pointer_dst_old < old_pos + insert_tokens.size()) {
				pointer_dst_new = token_index(prev_tokens, pointer_index_new + pointer_old);
			} else {
				pointer_dst_new = to_dst_index(pointer_dst_old);
			}
			PointerDataType pointer_new = pointer_dst_new - pointer_index_new;
			tokens[pointer_index_new].set_data<PointerDataType>(pointer_new);
		}
	}
}

void Program::exec_pending_ops() {
	for (ProgramCounterType op_index = 0; op_index < modify_ops.size(); op_index++) {
		ModifyOp& op = modify_ops[op_index];
		PointerDataType modify_index = to_dst_index(op.new_pos);
		Token current_token = op.new_token;
		tokens[modify_index] = current_token;
		if (op.recalc_pointers && op.new_token.is_ptr()) {
			PointerDataType pointer_index_old = op.old_pos;
			PointerDataType pointer_old = current_token.get_data_cast<PointerDataType>();
			PointerDataType pointer_dst_old = token_index(prev_tokens, pointer_index_old + pointer_old);
			PointerDataType pointer_index_new = op.new_pos;
			PointerDataType pointer_dst_new = to_dst_index(pointer_dst_old);
			PointerDataType pointer_new = pointer_dst_new - pointer_index_new;
			tokens[modify_index].set_data<PointerDataType>(pointer_new);
		}
	}
	for (ProgramCounterType op_index = 0; op_index < delete_ops.size(); op_index++) {
		DeleteOp& op = delete_ops[op_index];
		PointerDataType dst_erase_begin = to_dst_index(op.pos_begin);
		PointerDataType offset = calc_delete_offset(op.pos_begin, op.pos_end);
		shift_pointers(tokens, dst_erase_begin, offset);
		delete_op_exec(op.pos_begin, op.pos_end);
	}
	for (ProgramCounterType op_index = 0; op_index < insert_ops.size(); op_index++) {
		InsertOp& op = insert_ops[op_index];
		insert_op_exec(op.old_pos, op.new_pos, op.insert_tokens, OP_TYPE_NORMAL);
	}
	for (ProgramCounterType op_index = 0; op_index < replace_ops.size(); op_index++) {
		ReplaceOp& op = replace_ops[op_index];
		PointerDataType dst_erase_begin = to_dst_index(op.dst_begin);
		PointerDataType offset = calc_delete_offset(op.dst_begin, op.dst_end);
		shift_pointers(tokens, dst_erase_begin, offset + 1);
		shift_pointers(tokens, dst_erase_begin + 1, op.src_tokens.size() - 1);
		delete_op_exec(op.dst_begin, op.dst_end);
		insert_op_exec(op.src_begin, op.dst_begin, op.src_tokens, OP_TYPE_REPLACE);
	}
	for (ProgramCounterType op_index = 0; op_index < move_ops.size(); op_index++) {
		MoveOp& op = move_ops[op_index];
		std::vector<Token> src_tokens(prev_tokens.begin() + op.old_begin, prev_tokens.begin() + op.old_end);
		shift_pointers_move(tokens, op.old_begin, op.old_end, op.new_begin);
		delete_op_exec(op.old_begin, op.old_end);
		insert_op_exec(op.old_begin, op.new_begin, src_tokens, OP_TYPE_MOVE);
	}
}

void Program::print_node(Node* node, int indent_level) {
	std::string indent_string = "";
	if (node->token.is_list_end()) {
		indent_level--;
	}
	for (int j = 0; j < indent_level; j++) {
		indent_string += "    ";
	}
	std::cout << indent_string << node->to_string() << "\n";
	for (PointerDataType i = 0; i < node->arguments.size(); i++) {
		print_node(node->arguments[i].get(), indent_level + 1);
	}
}

PointerDataType Program::recalc_pointer(
	std::vector<Token>& parent_token_list,
	PointerDataType pointer_index, PointerDataType pointer,
	PointerDataType pos, PointerDataType offset,
	PointerDataType& index_new, PointerDataType& dst_new,
	bool pin_index, bool pin_dst
) {
	PointerDataType pointer_index_old = pointer_index;
	PointerDataType pointer_dst_old = token_index(parent_token_list, pointer_index + pointer);
	PointerDataType pointer_index_new = pointer_index_old;
	PointerDataType pointer_dst_new = pointer_dst_old;
	if (!pin_index) {
		if (offset < 0) {
			PointerDataType recalc_offset = std::clamp(
				(PointerDataType)pos - (PointerDataType)pointer_index_old, offset, (PointerDataType)0
			);
			if (pos <= pointer_index_old) {
				pointer_index_new += recalc_offset;
			}
		} else {
			if (pos <= pointer_index_old) {
				pointer_index_new += offset;
			}
		}
	}
	if (!pin_dst) {
		if (offset < 0) {
			PointerDataType recalc_offset = std::clamp(
				(PointerDataType)pos - (PointerDataType)pointer_dst_old, offset, (PointerDataType)0
			);
			if (pos <= pointer_dst_old) {
				pointer_dst_new += recalc_offset;
			}
		} else {
			if (pos <= pointer_dst_old) {
				pointer_dst_new += offset;
			}
		}
	}
	index_new = pointer_index_new;
	dst_new = pointer_dst_new;
	PointerDataType new_pointer = pointer_dst_new - pointer_index_new;
	return new_pointer;
}

PointerDataType Program::recalc_pointer_move(
	std::vector<Token>& token_list,
	PointerDataType pointer_index, PointerDataType pointer,
	PointerDataType old_begin, PointerDataType old_end,
	PointerDataType new_pos
) {
	PointerDataType pointer_index_old = pointer_index;
	PointerDataType pointer_dst_old = token_index(token_list, pointer_index + pointer);
	PointerDataType pointer_index_new = pointer_index_old;
	PointerDataType pointer_dst_new = pointer_dst_old;
	if (pointer_index_old >= old_begin && pointer_dst_old < old_end) {
		return pointer;
	}
	if (pointer_dst_old >= old_begin && pointer_dst_old < old_end) {
		PointerDataType offset = pointer_dst_old - old_begin;
		PointerDataType new_pointer = new_pos + offset;
		return new_pointer;
	}
	PointerDataType move_block_size = old_end - old_begin;
	PointerDataType new_index, new_dst;
	PointerDataType new_pointer_del = recalc_pointer(
		token_list, pointer_index, pointer, old_begin, -move_block_size,
		new_index, new_dst, false, false
	);
	PointerDataType new_pointer_ins = recalc_pointer(
		token_list, new_index, new_pointer_del, new_pos, move_block_size,
		new_index, new_dst, false, false
	);
	return new_pointer_ins;
}

void Program::_shift_pointers(
	std::vector<Token>& token_list, std::vector<Token>& parent_token_list,
	PointerDataType list_pos, PointerDataType pos, PointerDataType offset,
	bool pin_index, bool pin_dst
) {
	if (offset == 0) {
		return;
	}
	for (ProgramCounterType token_i = 0; token_i < token_list.size(); token_i++) {
		Token current_token = get_token(token_list, token_i);
		if (current_token.is_ptr()) {
			PointerDataType old_pointer = current_token.get_data_cast<PointerDataType>();
			PointerDataType new_index, new_dst;
			PointerDataType new_pointer = recalc_pointer(
				parent_token_list, list_pos + token_i, old_pointer, pos, offset, 
				new_index, new_dst, pin_index, pin_dst
			);
			get_token(token_list, token_i).set_data<PointerDataType>(new_pointer);
			get_token(token_list, token_i).str = get_token(token_list, token_i).to_string();
		}
	}
}

void Program::shift_pointers(std::vector<Token>& token_list,
	PointerDataType pos, PointerDataType offset
) {
	_shift_pointers(token_list, token_list, 0, pos, offset, false, false);
}

void Program::shift_pointers_rel(
	std::vector<Token>& token_list, std::vector<Token>& parent_token_list,
	ProgramCounterType list_pos, PointerDataType pos, PointerDataType offset,
	bool pin_index, bool pin_dst
) {
	_shift_pointers(token_list, parent_token_list, list_pos, pos, offset, pin_index, pin_dst);
}

void Program::shift_pointers_move(
	std::vector<Token>& token_list,
	ProgramCounterType old_begin, ProgramCounterType old_end,
	ProgramCounterType new_pos
) {
	for (ProgramCounterType token_i = 0; token_i < token_list.size(); token_i++) {
		Token current_token = get_token(token_list, token_i);
		if (current_token.is_ptr()) {
			PointerDataType old_pointer = current_token.get_data_cast<PointerDataType>();
			PointerDataType new_pointer = recalc_pointer_move(
				token_list, token_i, old_pointer, old_begin, old_end, new_pos
			);
			get_token(token_list, token_i).set_data<PointerDataType>(new_pointer);
			get_token(token_list, token_i).str = get_token(token_list, token_i).to_string();
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

void Program::binary_func(std::function<Token(Token, Token)> func) {
	if (rel_token(prev_tokens, 1).is_num_or_ptr() && rel_token(prev_tokens, 2).is_num_or_ptr()) {
		std::vector<Token> args = { rel_token(prev_tokens, 1), rel_token(prev_tokens, 2) };
		shift_pointers_rel(args, prev_tokens, program_counter + 1, program_counter, -3, false, true);
		Token result = func(args[0], args[1]);
		modify_token(program_counter, result);
		delete_tokens(program_counter + 1, program_counter + 3);
	}
}

std::vector<Token> Program::sys_call(int index, std::vector<Token> input) {
	switch (index) {
		case 0: return sys_print(input); break;
		default: return std::vector<Token>();
	}
}

std::vector<Token> Program::sys_print(std::vector<Token> input) {
	std::string str;
	for (Token token : input) {
		char c = token.get_data_cast<int>();
		str += c;
	}
	local_print_buffer += str;
	return std::vector<Token>();
}
