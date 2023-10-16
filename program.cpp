#include "program.h"

Program::NewPointersEntry::NewPointersEntry(PointerDataType index, PointerDataType pointer) {
	this->index = index;
	this->pointer = pointer;
}

Program::DeleteOp::DeleteOp(ProgramCounterType pos_begin, ProgramCounterType pos_end, OpPriority priority) {
	this->pos_begin = pos_begin;
	this->pos_end = pos_end;
	this->priority = priority;
}

Program::InsertOp::InsertOp(ProgramCounterType dst_pos, std::vector<Token> insert_tokens) {
	this->src_pos = -1;
	this->dst_pos = dst_pos;
	this->insert_tokens = insert_tokens;
	this->recalc_pointers = false;
}

Program::InsertOp::InsertOp(ProgramCounterType src_pos, ProgramCounterType dst_pos, std::vector<Token> insert_tokens) {
	this->src_pos = src_pos;
	this->dst_pos = dst_pos;
	this->insert_tokens = insert_tokens;
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
					Label new_label(current_word.substr(1, current_word.size() - 1), (PointerDataType)i - 1);
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
			reset_index_shift();
			local_print_buffer = "";
			parse();
			auto jump_to_end = [&]() {
				Node* seq_node = node_pointers[list_scope_stack.top().pos];
				program_counter = seq_node->last_index;
			};
			auto notify_parent = [&]() {
				if (list_scope_stack.size() > 0) {
					list_scope_stack.top().instruction_executed = true;
				}
			};
			auto exit_parent = [&]() {
				jump_to_end();
				list_scope_stack.pop();
				notify_parent();
			};
			auto exec_end = [&]() {
				jump_to_end();
				execute_instruction();
				notify_parent();
			};
			auto exec_normal = [&]() {
				execute_instruction();
				list_scope_stack.top().instruction_executed = true;
			};
			auto exec_silent = [&]() {
				execute_instruction();
			};
			for (program_counter = 0; program_counter < prev_tokens.size(); program_counter++) {
				Token current_token = rel_token(prev_tokens, 0);
				if (parent_is_seq() && list_scope_stack.top().instruction_executed) {
					exit_parent();
				} else if (current_token.is_num_or_ptr()) {
					// skipping
				} else if (current_token.str == "seq" || current_token.str == "list") {
					exec_silent();
				} else if (current_token.str == "q") {
					exec_silent();
				} else {
					if (parent_is_seq()) {
						if (current_token.str == "end") {
			   				exec_end();
			   			} else {
			   				exec_normal();
			   			}
					} else if (parent_is_list()) {
						if (current_token.str == "end") {
							if (!list_scope_stack.top().instruction_executed) {
								exec_end();
							} else {
								exit_parent();
							}
						} else {
							exec_normal();
						}
					} else {
						exec_silent();
					}
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

void Program::execute_instruction() {
	Token current_token = rel_token(prev_tokens, 0);
	if (current_token.str == "add") {
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
			ProgramCounterType src_index_begin = token_index(prev_tokens, program_counter + 1 + src);
			delete_tokens(program_counter, program_counter + 3, OP_PRIORITY_WEAK_DELETE);
			if (src_index_begin != prev_tokens.size()) {
				ProgramCounterType dst_index = token_index(prev_tokens, program_counter + 2 + dst);
				Node* node = node_pointers[token_index(prev_tokens, src_index_begin)];
				std::vector<Token> node_tokens = node->tokenize();
				insert_tokens(src_index_begin, dst_index, node_tokens);
			}
		}
	} else if (current_token.str == "del") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr()) {
			PointerDataType arg = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			ProgramCounterType target_index = token_index(prev_tokens, program_counter + 1 + arg);
			delete_tokens(program_counter, program_counter + 2, OP_PRIORITY_WEAK_DELETE);
			if (target_index != prev_tokens.size()) {
				Node* node = node_pointers[token_index(prev_tokens, target_index)];
				std::vector<Token> node_tokens = node->tokenize();
				delete_tokens(target_index, target_index + node_tokens.size(), OP_PRIORITY_STRONG_DELETE);
			}
		}
	} else if (current_token.str == "get") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr()) {
			PointerDataType src = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			PointerDataType src_index_begin = token_index(prev_tokens, program_counter + 1 + src);
			if (src_index_begin != prev_tokens.size()) {
				Node* src_node = node_pointers[token_index(prev_tokens, src_index_begin)];
				std::vector<Token> src_node_tokens = src_node->tokenize();
				replace_tokens(program_counter, program_counter + 2, src_index_begin, src_node_tokens);
			} else {
				delete_tokens(program_counter, program_counter + 2, OP_PRIORITY_WEAK_DELETE);
			}
		}
	} else if (current_token.str == "set") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr()) {
			PointerDataType dst = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			ProgramCounterType src_index_begin = program_counter + 2;
			ProgramCounterType dst_index_begin = token_index(prev_tokens, program_counter + 1 + dst);
			Node* src_node = node_pointers[token_index(prev_tokens, src_index_begin)];
			std::vector<Token> src_node_tokens = src_node->tokenize();
			delete_tokens(program_counter, program_counter + 2 + src_node_tokens.size(), OP_PRIORITY_WEAK_DELETE);
			if (dst_index_begin != prev_tokens.size()) {
				Node* dst_node = node_pointers[token_index(prev_tokens, dst_index_begin)];
				std::vector<Token> dst_node_tokens = dst_node->tokenize();
				replace_tokens(dst_index_begin, dst_index_begin + dst_node_tokens.size(), src_index_begin, src_node_tokens);
			}
			program_counter = node_pointers[program_counter]->last_index;
		}
	} else if (current_token.str == "ins") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr()) {
			PointerDataType dst = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			ProgramCounterType src_index_begin = program_counter + 2;
			ProgramCounterType dst_index_begin = token_index(prev_tokens, program_counter + 1 + dst);
			Node* src_node = node_pointers[token_index(prev_tokens, src_index_begin)];
			std::vector<Token> src_node_tokens = src_node->tokenize();
			delete_tokens(program_counter, program_counter + 2 + src_node_tokens.size(), OP_PRIORITY_WEAK_DELETE);
			insert_tokens(src_index_begin, dst_index_begin, src_node_tokens);
			program_counter = node_pointers[program_counter]->last_index;
		}
	} else if (current_token.str == "repl") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr() && rel_token(prev_tokens, 2).is_num_or_ptr()) {
			PointerDataType dst = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			PointerDataType src = rel_token(prev_tokens, 2).get_data_cast<PointerDataType>();
			ProgramCounterType dst_index_begin = token_index(prev_tokens, program_counter + 1 + dst);
			ProgramCounterType src_index_begin = token_index(prev_tokens, program_counter + 2 + src);
			delete_tokens(program_counter, program_counter + 3, OP_PRIORITY_WEAK_DELETE);
			if (src_index_begin != prev_tokens.size() && dst_index_begin != prev_tokens.size()) {
				Node* dst_node = node_pointers[token_index(prev_tokens, dst_index_begin)];
				Node* src_node = node_pointers[token_index(prev_tokens, src_index_begin)];
				ProgramCounterType dst_index_end = dst_node->last_index + 1;
				std::vector<Token> src_node_tokens = src_node->tokenize();
				replace_tokens(dst_index_begin, dst_index_end, src_index_begin, src_node_tokens);
			}
		}
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
	} else if (current_token.str == "list") {
		list_scope_stack.push({ program_counter, false });
	} else if (current_token.str == "seq") {
		list_scope_stack.push({ program_counter, false });
	} else if (current_token.str == "end") {
		ProgramCounterType list_pos = list_scope_stack.top().pos;
		delete_tokens(list_pos, list_pos + 1, OP_PRIORITY_STRONG_DELETE);
		delete_tokens(program_counter, program_counter + 1, OP_PRIORITY_STRONG_DELETE);
		list_scope_stack.pop();
	} else if (current_token.str == "q") {
		Node* node = node_pointers[program_counter];
		program_counter = node->last_index;
	} else {
		throw std::runtime_error("Unexpected token: " + current_token.str);
	}
}

void Program::parse() {
	node_pointers = std::vector<Node*>();
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
	node_pointers.push_back(new_node_p);
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
		if (current_token.str == "list" || current_token.str == "seq") {
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
	new_node_p->last_index = parse_token_index;
	new_token_index = parse_token_index;
	return new_node;
}

bool Program::parent_is_seq() {
	return 
		list_scope_stack.size() > 0 
		&& get_token(prev_tokens, list_scope_stack.top().pos).str == "seq"
	;
}

bool Program::parent_is_list() {
	return
		list_scope_stack.size() > 0
		&& get_token(prev_tokens, list_scope_stack.top().pos).str == "list"
	;
}

bool Program::IndexShiftEntry::is_deleted() {
	return op_priority == OP_PRIORITY_WEAK_DELETE
		|| op_priority == OP_PRIORITY_STRONG_DELETE
	;
}

bool Program::IndexShiftEntry::is_weakly_deleted() {
	return op_priority == OP_PRIORITY_WEAK_DELETE;
}

bool Program::IndexShiftEntry::is_strongly_deleted() {
	return op_priority == OP_PRIORITY_STRONG_DELETE;
}

bool Program::IndexShiftEntry::is_replaced() {
	return 
		op_priority == OP_PRIORITY_REPLACE 
		|| op_priority == OP_PRIORITY_FUNC_REPLACE
	;
}

bool Program::IndexShiftEntry::is_weakly_replaced() {
	return op_priority == OP_PRIORITY_FUNC_REPLACE;
}

bool Program::IndexShiftEntry::is_strongly_replaced() {
	return op_priority == OP_PRIORITY_REPLACE;
}

bool Program::IndexShiftEntry::is_untouched() {
	return op_priority == OP_PRIORITY_NONE;
}

bool Program::IndexShiftEntry::is_temp() {
	return op_priority == OP_PRIORITY_TEMP;
}

bool Program::IndexShiftEntry::is_not_temp() {
	return op_priority != OP_PRIORITY_TEMP;
}

PointerDataType Program::to_dst_index(PointerDataType old_index) {
	return index_shift[old_index].index;
}

PointerDataType Program::to_src_index(PointerDataType new_index) {
	return index_shift_rev[new_index];
}

void Program::insert_op_exec(PointerDataType old_src_pos, ProgramCounterType old_dst_pos, std::vector<Token> insert_tokens, OpType op_type) {
	PointerDataType offset = insert_tokens.size();
	PointerDataType new_dst_pos = -1;
	for (ProgramCounterType i = old_dst_pos; i < index_shift.size(); i++) {
		if (new_dst_pos == -1) {
			if (
				index_shift[i].is_temp()
				|| (op_type == OP_TYPE_REPLACE && index_shift[i].is_weakly_deleted())
			) {
				new_dst_pos = index_shift[i].index;
			} else if (index_shift[i].is_untouched()) {
				new_dst_pos = index_shift[i].index;
				index_shift[i].index += offset;
			}
		} else {
			if (index_shift[i].index >= 0 && index_shift[i].is_not_temp()) {
				index_shift[i].index += offset;
			}
		}
	}
	std::vector<PointerDataType> ins_vector(offset);
	for (ProgramCounterType i = 0; i < offset; i++) {
		PointerDataType current_pos = old_src_pos + i;
		Token& current_token = insert_tokens[i];
		if (current_token.is_ptr()) {
			PointerDataType pointer = current_token.get_data_cast<PointerDataType>();
			PointerDataType old_dst = token_index(prev_tokens, current_pos + pointer);
			if (old_dst >= old_src_pos && old_dst < old_src_pos + offset) {
				ins_vector[i] = -1;
			} else {
				ins_vector[i] = current_pos;
			}
		} else {
			ins_vector[i] = -1;
		}
	}
	index_shift_rev.insert(index_shift_rev.begin() + new_dst_pos, ins_vector.begin(), ins_vector.end());
	tokens.insert(tokens.begin() + new_dst_pos, insert_tokens.begin(), insert_tokens.end());
}

PointerDataType Program::delete_op_exec(ProgramCounterType old_pos_begin, ProgramCounterType old_pos_end, OpType op_type) {
	PointerDataType new_pos_begin = to_dst_index(old_pos_begin);
	if (new_pos_begin < 0) {
		return 0;
	}
	PointerDataType offset = 0;
	for (ProgramCounterType i = old_pos_begin; i < old_pos_end; i++) {
		index_shift[i].index = index_shift[old_pos_begin].index;
		if (!index_shift[i].is_deleted()) {
			offset++;
		}
		if (index_shift[i].is_untouched()) {
			index_shift[i].op_priority = OP_PRIORITY_TEMP;
		}
	}
	for (ProgramCounterType i = old_pos_end; i < index_shift.size(); i++) {
		if (index_shift[i].index >= 0) {
			index_shift[i].index -= offset;
		}
	}
	PointerDataType new_pos_end = new_pos_begin + offset;
	index_shift_rev.erase(index_shift_rev.begin() + new_pos_begin, index_shift_rev.begin() + new_pos_end);
	tokens.erase(tokens.begin() + new_pos_begin, tokens.begin() + new_pos_end);
	return offset;
}

void Program::delete_tokens(ProgramCounterType pos_begin, ProgramCounterType pos_end, OpPriority priority) {
	delete_ops.push_back(DeleteOp(pos_begin, pos_end, priority));
}

void Program::insert_tokens(ProgramCounterType old_pos, ProgramCounterType new_pos, std::vector<Token> insert_tokens) {
	insert_ops.push_back(InsertOp(old_pos, new_pos, insert_tokens));
}

void Program::replace_tokens(
	ProgramCounterType dst_begin, ProgramCounterType dst_end,
	ProgramCounterType src_begin, std::vector<Token> src_tokens
) {
	replace_ops.push_back(ReplaceOp(dst_begin, dst_end, src_begin, src_tokens));
}

void Program::replace_tokens_func(
	ProgramCounterType dst_begin, ProgramCounterType dst_end,
	ProgramCounterType src_begin, std::vector<Token> src_tokens
) {
	func_replace_ops.push_back(ReplaceOp(dst_begin, dst_end, src_begin, src_tokens));
}

void Program::exec_replace_ops(std::vector<ReplaceOp>& vec, OpPriority priority) {
	for (ReplaceOp& op : vec | std::views::reverse) {
		if (index_shift[op.dst_begin].op_priority >= priority) {
			continue;
		}
		delete_op_exec(op.dst_begin, op.dst_end, OP_TYPE_REPLACE);
		insert_op_exec(op.src_begin, op.dst_begin, op.src_tokens, OP_TYPE_REPLACE);
		for (ProgramCounterType token_i = op.dst_begin; token_i < op.dst_end; token_i++) {
			index_shift[token_i].op_priority = priority;
		}
	}
}

void Program::exec_pending_ops() {
	for (ProgramCounterType op_index = 0; op_index < delete_ops.size(); op_index++) {
		DeleteOp& op = delete_ops[op_index];
		if (index_shift[op.pos_begin].is_strongly_deleted()) {
			continue;
		}
		delete_op_exec(op.pos_begin, op.pos_end, OP_TYPE_NORMAL);
		for (ProgramCounterType token_i = op.pos_begin; token_i < op.pos_end; token_i++) {
			index_shift[token_i].op_priority = op.priority;
		}
	}
	for (ProgramCounterType op_index = 0; op_index < insert_ops.size(); op_index++) {
		InsertOp& op = insert_ops[op_index];
		insert_op_exec(op.src_pos, op.dst_pos, op.insert_tokens, OP_TYPE_NORMAL);
	}
	exec_replace_ops(replace_ops, OP_PRIORITY_REPLACE);
	exec_replace_ops(func_replace_ops, OP_PRIORITY_FUNC_REPLACE);
	shift_pointers();
}

void Program::reset_index_shift() {
	try {
		if (tokens.size() != prev_tokens.size()) {
			throw std::runtime_error("tokens.size() != prev_tokens.size()");
		}
		index_shift = std::vector<IndexShiftEntry>(prev_tokens.size() + 1);
		index_shift_rev = std::vector<PointerDataType>(prev_tokens.size() + 1);
		for (ProgramCounterType i = 0; i < index_shift.size(); i++) {
			index_shift[i].index = i;
			index_shift_rev[i] = i;
		}
		delete_ops.clear();
		insert_ops.clear();
		replace_ops.clear();
		func_replace_ops.clear();
		move_ops.clear();
		new_pointers.clear();
		list_scope_stack = std::stack<ListScopeStackEntry>();
	} catch (std::exception exc) {
		throw std::runtime_error(__FUNCTION__": " + std::string(exc.what()));
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

void Program::shift_pointers() {
	for (ProgramCounterType token_i = 0; token_i < tokens.size(); token_i++) {
		Token& current_token = tokens[token_i];
		if (current_token.is_ptr()) {
			PointerDataType new_index = token_i;
			PointerDataType old_index = to_src_index(new_index);
			if (old_index < 0) {
				continue;
			}
			PointerDataType old_pointer;
			auto it = new_pointers.find(NewPointersEntry(old_index, 0));
			if (it != new_pointers.end()) {
				old_pointer = (*it).pointer;
			} else {
				old_pointer = prev_tokens[old_index].get_data_cast<PointerDataType>();
			}
			PointerDataType old_dst = token_index(prev_tokens, old_index + old_pointer);
			PointerDataType new_dst;
			for (PointerDataType dst_i = old_dst; ; dst_i++) {
				PointerDataType dst_p = to_dst_index(dst_i);
				if (!index_shift[dst_i].is_deleted()) {
					new_dst = dst_p;
					break;
				}
			}
			PointerDataType new_pointer = new_dst - new_index;
			current_token.set_data<PointerDataType>(new_pointer);
			current_token.str = current_token.to_string();
		}
	}
}

void Program::unary_func(std::function<Token(Token)> func) {
	Token& arg = rel_token(prev_tokens, 1);
	if (arg.is_ptr()) {
		arg.set_data<PointerDataType>(arg.get_data<PointerDataType>() + 1);
	}
	Token result = func(arg);
	new_pointers.insert(NewPointersEntry(program_counter, result.get_data_cast<PointerDataType>()));
	replace_tokens_func(program_counter, program_counter + 2, program_counter, { result });
}

void Program::binary_func(std::function<Token(Token, Token)> func) {
	if (rel_token(prev_tokens, 1).is_num_or_ptr() && rel_token(prev_tokens, 2).is_num_or_ptr()) {
		Token& arg1 = rel_token(prev_tokens, 1);
		Token& arg2 = rel_token(prev_tokens, 2);
		if (arg1.is_ptr()) {
			arg1.set_data<PointerDataType>(arg1.get_data<PointerDataType>() + 1);
		}
		if (arg2.is_ptr()) {
			arg2.set_data<PointerDataType>(arg2.get_data<PointerDataType>() + 2);
		}
		Token result = func(arg1, arg2);
		new_pointers.insert(NewPointersEntry(program_counter, result.get_data_cast<PointerDataType>()));
		replace_tokens_func(program_counter, program_counter + 3, program_counter, { result });
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

bool operator<(const Program::NewPointersEntry& left, const Program::NewPointersEntry& right) {
	return left.index < right.index;
}