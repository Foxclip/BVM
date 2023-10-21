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

Program::MoveReplaceOp::MoveReplaceOp(
	ProgramCounterType old_begin, ProgramCounterType old_end,
	ProgramCounterType new_begin, ProgramCounterType new_end
) {
	this->old_begin = old_begin;
	this->old_end = old_end;
	this->new_begin = new_begin;
	this->new_end = new_end;
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

Node::Node() { }

std::vector<Token> Node::tokenize(std::vector<Node>& nodes, std::vector<Token>& tokens) {
	std::vector<Token> result_tokens;
	result_tokens.push_back(tokens[first_index]);
	for (ProgramCounterType i = 0; i < arguments.size(); i++) {
		std::vector<Token> arg_tokens = nodes[arguments[i]].tokenize(nodes, tokens);
		result_tokens.insert(result_tokens.end(), arg_tokens.begin(), arg_tokens.end());
	}
	return result_tokens;
}

Node* Node::get_parent(std::vector<Node>& nodes) {
	return &nodes[nodes[first_index].parent_index];
}

ProgramCounterType Node::get_parent_count(std::vector<Node>& nodes) {
	ProgramCounterType count = 0;
	Node* current_node = this;
	while (current_node->parent_index >= 0) {
		count++;
		current_node = current_node->get_parent(nodes);
	}
	return count;
}

Token* Node::get_token(std::vector<Token>& tokens) {
	return &tokens[first_index];
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
		print_node(&nodes[i]);
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
				Node* seq_node = &nodes[list_scope_stack.top().pos];
				program_counter = seq_node->last_index;
			};
			auto notify_container = [&]() {
				if (list_scope_stack.size() > 0) {
					list_scope_stack.top().instruction_executed = true;
				}
			};
			auto exit_parent = [&]() {
				jump_to_end();
				list_scope_stack.pop();
				notify_container();
			};
			auto try_exec_normal = [&]() {
				if (try_execute_instruction()) {
					notify_container();
				}
			};
			auto try_exec_end = [&]() {
				jump_to_end(); // ???
				try_exec_normal();
			};
			auto try_exec_silent = [&]() {
				try_execute_instruction();
			};
			for (program_counter = 0; program_counter < prev_tokens.size(); program_counter++) {
				Token& current_token = prev_tokens[program_counter];
				if (parent_is_seq() && list_scope_stack.top().instruction_executed) {
					exit_parent();
				} else if (current_token.is_num_or_ptr()) {
					// skipping
				} else if (current_token.str == "seq" || current_token.str == "list") {
					try_exec_silent();
				} else if (current_token.str == "q") {
					try_exec_silent();
				} else {
					if (inside_seq()) {
						if (current_token.str == "end") {
			   				try_exec_end();
			   			} else {
			   				try_exec_normal();
			   			}
					} else if (inside_list()) {
						if (current_token.str == "end") {
							if (!list_scope_stack.top().instruction_executed) {
								try_exec_end();
							} else {
								exit_parent();
							}
						} else {
							try_exec_normal();
						}
					} else {
						try_exec_silent();
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

bool Program::try_execute_instruction() {
	Token current_token = rel_token(prev_tokens, 0);
	if (current_token.str == "add") {
		binary_func([](Token a, Token b) { return Token::add(a, b); });
		return true;
	} else if (current_token.str == "sub") {
		binary_func([](Token a, Token b) { return Token::sub(a, b); });
		return true;
	} else if (current_token.str == "mul") {
		binary_func([](Token a, Token b) { return Token::mul(a, b); });
		return true;
	} else if (current_token.str == "div") {
		binary_func([](Token a, Token b) { return Token::div(a, b); });
		return true;
	} else if (current_token.str == "mod") {
		binary_func([](Token a, Token b) { return Token::mod(a, b); });
		return true;
	} else if (current_token.str == "pow") {
		binary_func([](Token a, Token b) { return Token::pow(a, b); });
		return true;
	} else if (current_token.str == "log") {
		unary_func([](Token a) { return Token::log(a); });
		return true;
	} else if (current_token.str == "log2") {
		unary_func([](Token a) { return Token::log2(a); });
		return true;
	} else if (current_token.str == "sin") {
		unary_func([](Token a) { return Token::sin(a); });
		return true;
	} else if (current_token.str == "cos") {
		unary_func([](Token a) { return Token::cos(a); });
		return true;
	} else if (current_token.str == "tan") {
		unary_func([](Token a) { return Token::tan(a); });
		return true;
	} else if (current_token.str == "asin") {
		unary_func([](Token a) { return Token::asin(a); });
		return true;
	} else if (current_token.str == "acos") {
		unary_func([](Token a) { return Token::acos(a); });
		return true;
	} else if (current_token.str == "atan") {
		unary_func([](Token a) { return Token::atan(a); });
		return true;
	} else if (current_token.str == "atan2") {
		binary_func([](Token a, Token b) { return Token::atan2(a, b); });
		return true;
	} else if (current_token.str == "floor") {
		unary_func([](Token a) { return Token::floor(a); });
		return true;
	} else if (current_token.str == "ceil") {
		unary_func([](Token a) { return Token::ceil(a); });
		return true;
	} else if (current_token.str == "cmp") {
		binary_func([](Token a, Token b) { return Token::cmp(a, b); });
		return true;
	} else if (current_token.str == "lt") {
		binary_func([](Token a, Token b) { return Token::lt(a, b); });
		return true;
	} else if (current_token.str == "gt") {
		binary_func([](Token a, Token b) { return Token::gt(a, b); });
		return true;
	} else if (current_token.str == "and") {
		binary_func([](Token a, Token b) { return Token::and_op(a, b); });
		return true;
	} else if (current_token.str == "or") {
		binary_func([](Token a, Token b) { return Token::or_op(a, b); });
		return true;
	} else if (current_token.str == "xor") {
		binary_func([](Token a, Token b) { return Token::xor_op(a, b); });
		return true;
	} else if (current_token.str == "not") {
		unary_func([](Token a) { return Token::not_op(a); });
		return true;
	} else if (current_token.str == "cpy") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr() && rel_token(prev_tokens, 2).is_num_or_ptr()) {
			PointerDataType src = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			PointerDataType dst = rel_token(prev_tokens, 2).get_data_cast<PointerDataType>();
			ProgramCounterType src_index_begin = token_index(prev_tokens, program_counter + 1 + src);
			delete_tokens(program_counter, program_counter + 3, OP_PRIORITY_WEAK_DELETE);
			if (src_index_begin != prev_tokens.size()) {
				ProgramCounterType dst_index = token_index(prev_tokens, program_counter + 2 + dst);
				Node* node = &nodes[token_index(prev_tokens, src_index_begin)];
				std::vector<Token> node_tokens = node->tokenize(nodes, tokens);
				insert_tokens(src_index_begin, dst_index, node_tokens);
			}
			return true;
		}
		return false;
	} else if (current_token.str == "del") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr()) {
			PointerDataType arg = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			ProgramCounterType target_index = token_index(prev_tokens, program_counter + 1 + arg);
			delete_tokens(program_counter, program_counter + 2, OP_PRIORITY_WEAK_DELETE);
			if (target_index != prev_tokens.size()) {
				Node* node = &nodes[token_index(prev_tokens, target_index)];
				std::vector<Token> node_tokens = node->tokenize(nodes, tokens);
				delete_tokens(target_index, target_index + node_tokens.size(), OP_PRIORITY_STRONG_DELETE);
			}
			return true;
		}
		return false;
	} else if (current_token.str == "get") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr()) {
			PointerDataType src = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			PointerDataType src_index_begin = token_index(prev_tokens, program_counter + 1 + src);
			if (src_index_begin != prev_tokens.size()) {
				Node* src_node = &nodes[token_index(prev_tokens, src_index_begin)];
				std::vector<Token> src_node_tokens = src_node->tokenize(nodes, tokens);
				replace_tokens(program_counter, program_counter + 2, src_index_begin, src_node_tokens);
			} else {
				delete_tokens(program_counter, program_counter + 2, OP_PRIORITY_WEAK_DELETE);
			}
			return true;
		}
		return false;
	} else if (current_token.str == "set") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr() && rel_token(prev_tokens, 2).is_static()) {
			PointerDataType dst = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			ProgramCounterType src_index_begin = program_counter + 2;
			ProgramCounterType dst_index_begin = token_index(prev_tokens, program_counter + 1 + dst);
			delete_tokens(program_counter, nodes[program_counter].last_index + 1, OP_PRIORITY_WEAK_DELETE);
			if (dst_index_begin != prev_tokens.size()) {
				Node* src_node = &nodes[token_index(prev_tokens, src_index_begin)];
				if (tokens[src_node->first_index].str == "q") {
					src_node = &nodes[src_node->arguments[0]];
				}
				Node* dst_node = &nodes[token_index(prev_tokens, dst_index_begin)];
				std::vector<Token> src_node_tokens = src_node->tokenize(nodes, tokens);
				std::vector<Token> dst_node_tokens = dst_node->tokenize(nodes, tokens);
				replace_tokens(dst_index_begin, dst_node->last_index + 1, src_index_begin, src_node_tokens);
			}
			return true;
		}
		return false;
	} else if (current_token.str == "ins") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr() && rel_token(prev_tokens, 2).is_static()) {
			PointerDataType dst = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			ProgramCounterType src_index_begin = program_counter + 2;
			ProgramCounterType dst_index_begin = token_index(prev_tokens, program_counter + 1 + dst);
			Node* src_node = &nodes[token_index(prev_tokens, src_index_begin)];
			if (tokens[src_node->first_index].str == "q") {
				src_node = &nodes[src_node->arguments[0]];
			}
			std::vector<Token> src_node_tokens = src_node->tokenize(nodes, tokens);
			delete_tokens(program_counter, nodes[program_counter].last_index + 1, OP_PRIORITY_WEAK_DELETE);
			insert_tokens(src_index_begin, dst_index_begin, src_node_tokens);
			return true;
		}
		return false;
	} else if (current_token.str == "repl") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr() && rel_token(prev_tokens, 2).is_num_or_ptr()) {
			PointerDataType dst = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			PointerDataType src = rel_token(prev_tokens, 2).get_data_cast<PointerDataType>();
			ProgramCounterType dst_index_begin = token_index(prev_tokens, program_counter + 1 + dst);
			ProgramCounterType src_index_begin = token_index(prev_tokens, program_counter + 2 + src);
			delete_tokens(program_counter, program_counter + 3, OP_PRIORITY_WEAK_DELETE);
			if (src_index_begin != prev_tokens.size() && dst_index_begin != prev_tokens.size()) {
				Node* dst_node = &nodes[token_index(prev_tokens, dst_index_begin)];
				Node* src_node = &nodes[token_index(prev_tokens, src_index_begin)];
				ProgramCounterType dst_index_end = dst_node->last_index + 1;
				std::vector<Token> src_node_tokens = src_node->tokenize(nodes, tokens);
				replace_tokens(dst_index_begin, dst_index_end, src_index_begin, src_node_tokens);
			}
			return true;
		}
		return false;
	} else if (current_token.str == "move") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr() && rel_token(prev_tokens, 2).is_num_or_ptr()) {
			PointerDataType src = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			PointerDataType dst = rel_token(prev_tokens, 2).get_data_cast<PointerDataType>();
			ProgramCounterType src_index_begin = token_index(prev_tokens, program_counter + 1 + src);
			ProgramCounterType dst_index_begin = token_index(prev_tokens, program_counter + 2 + dst);
			delete_tokens(program_counter, program_counter + 3, OP_PRIORITY_WEAK_DELETE);
			if (src_index_begin != prev_tokens.size()) {
				Node* src_node = &nodes[token_index(prev_tokens, src_index_begin)];
				move_tokens(src_index_begin, src_node->last_index + 1, dst_index_begin);
			}
			return true;
		}
		return false;
	} else if (current_token.str == "mrep") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr() && rel_token(prev_tokens, 2).is_num_or_ptr()) {
			PointerDataType src = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			PointerDataType dst = rel_token(prev_tokens, 2).get_data_cast<PointerDataType>();
			ProgramCounterType src_index_begin = token_index(prev_tokens, program_counter + 1 + src);
			ProgramCounterType dst_index_begin = token_index(prev_tokens, program_counter + 2 + dst);
			delete_tokens(program_counter, program_counter + 3, OP_PRIORITY_WEAK_DELETE);
			if (src_index_begin != prev_tokens.size() && dst_index_begin != prev_tokens.size()) {
				Node* src_node = &nodes[token_index(prev_tokens, src_index_begin)];
				Node* dst_node = &nodes[token_index(prev_tokens, dst_index_begin)];
				movereplace_tokens(
					src_index_begin, src_node->last_index + 1,
					dst_index_begin, dst_node->last_index + 1
				);
			}
			return true;
		}
		return false;
	} else if (current_token.str == "if") {
		if (rel_token(tokens, 1).is_num_or_ptr()) {
			BoolType cond = rel_token(tokens, 1).get_data_cast<BoolType>();
			Node* if_node = &nodes[token_index(tokens, program_counter)];
			Node* true_node = &nodes[if_node->arguments[1]];
			Node* false_node = &nodes[if_node->arguments[2]];
			Node* selected_node = cond != 0 ? true_node : false_node;
			if (tokens[selected_node->first_index].str == "q") {
				selected_node = &nodes[selected_node->arguments[0]];
			}
			movereplace_tokens(
				selected_node->first_index, selected_node->last_index + 1,
				if_node->first_index, if_node->last_index + 1
			);
			return true;
		}
		return false;
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
		return true;
	} else if (current_token.str == "seq") {
		list_scope_stack.push({ program_counter, false });
		return true;
	} else if (current_token.str == "end") {
		ProgramCounterType list_pos = list_scope_stack.top().pos;
		delete_tokens(list_pos, list_pos + 1, OP_PRIORITY_LIST_DELETE);
		delete_tokens(program_counter, program_counter + 1, OP_PRIORITY_LIST_DELETE);
		list_scope_stack.pop();
		return true;
	} else if (current_token.str == "q") {
		Node* node = &nodes[program_counter];
		program_counter = node->last_index;
		return true;
	} else {
		throw std::runtime_error("Unexpected token: " + current_token.str);
	}
}

void Program::parse() {
	try {
		PointerDataType diff = tokens.size() - nodes.size();
		for (PointerDataType i = 0; i < diff; i++) {
			nodes.push_back(Node());
		}
		std::stack<PointerDataType> parent_stack;
		for (PointerDataType token_i = 0; token_i < tokens.size(); token_i++) {
			Token& current_token = tokens[token_i];
			Node* new_node_p = &nodes[token_i];
			new_node_p->parent_index = -1;
			new_node_p->arg_count = 0;
			new_node_p->arguments.clear(); // might reinitialize instead for saving memory
			new_node_p->first_index = token_i;
			new_node_p->last_index = 0;
			if (!parent_stack.empty()) {
				new_node_p->parent_index = parent_stack.top();
				nodes[parent_stack.top()].arguments.push_back(token_i);
			}
			ProgramCounterType arg_count = 0;
			if (!current_token.is_num_or_ptr()) {
				arg_count = get_arg_count(current_token.get_data_cast<InstructionDataType>());
			}
			new_node_p->arg_count = arg_count;
			if (arg_count > 0) {
				parent_stack.push(token_i);
			} else {
				new_node_p->last_index = token_i;
			}
			ProgramCounterType current_index = token_i;
			ProgramCounterType current_last_index;
			bool first = true;
			while (!parent_stack.empty()) {
				Node& current_parent = nodes[parent_stack.top()];
				ProgramCounterType arg_offset = current_index - current_parent.first_index;
				bool arg_offset_end = arg_offset >= current_parent.arg_count;
				bool end_end = tokens[current_index].str == "end";
				auto exit_level = [&]() {
					if (first) {
						current_last_index = current_index;
						first = false;
					}
					nodes[parent_stack.top()].last_index = current_last_index;
					current_index = nodes[parent_stack.top()].first_index;
					parent_stack.pop();
				};
				if (arg_offset_end || end_end) {
					exit_level();
				} else {
					break;
				}
			}
		}
	} catch (std::exception exc) {
		throw std::runtime_error(__FUNCTION__": " + std::string(exc.what()));
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

bool Program::inside_seq() {
	return
		list_scope_stack.size() > 0
		&& get_token(prev_tokens, list_scope_stack.top().pos).str == "seq"
	;
}

bool Program::inside_list() {
	return
		list_scope_stack.size() > 0
		&& get_token(prev_tokens, list_scope_stack.top().pos).str == "list"
	;
}

bool Program::parent_is_seq() {
	PointerDataType parent_index = nodes[program_counter].parent_index;
	return parent_index >= 0 && nodes[parent_index].get_token(tokens)->str == "seq";
}

bool Program::parent_is_list() {
	PointerDataType parent_index = nodes[program_counter].parent_index;
	return parent_index >= 0 && nodes[parent_index].get_token(tokens)->str == "list";
}

bool Program::parent_is_if() {
	PointerDataType parent_index = nodes[program_counter].parent_index;
	return parent_index >= 0 && nodes[parent_index].get_token(tokens)->str == "if";
}

bool Program::IndexShiftEntry::is_deleted() {
	return op_priority == OP_PRIORITY_LIST_DELETE
		|| op_priority == OP_PRIORITY_WEAK_DELETE
		|| op_priority == OP_PRIORITY_STRONG_DELETE
	;
}

bool Program::IndexShiftEntry::is_list_deleted() {
	return op_priority == OP_PRIORITY_LIST_DELETE;
}

bool Program::IndexShiftEntry::is_weakly_deleted() {
	return op_priority == OP_PRIORITY_WEAK_DELETE;
}

bool Program::IndexShiftEntry::is_strongly_deleted() {
	return op_priority == OP_PRIORITY_STRONG_DELETE;
}

bool Program::IndexShiftEntry::is_moved() {
	return is_normal_moved() || is_mrep_src();
}

bool Program::IndexShiftEntry::is_normal_moved() {
	return op_priority == OP_PRIORITY_MOVE;
}

bool Program::IndexShiftEntry::is_replaced() {
	return 
		op_priority == OP_PRIORITY_REPLACE 
		|| op_priority == OP_PRIORITY_FUNC_REPLACE
	;
}

bool Program::IndexShiftEntry::is_mrep_src() {
	return op_priority == OP_PRIORITY_MREP_SRC;
}

bool Program::IndexShiftEntry::is_weakly_replaced() {
	return op_priority == OP_PRIORITY_FUNC_REPLACE;
}

bool Program::IndexShiftEntry::is_strongly_replaced() {
	return op_priority == OP_PRIORITY_REPLACE;
}

bool Program::IndexShiftEntry::is_untouched() {
	return op_priority == OP_PRIORITY_NULL;
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
				|| (
					(op_type == OP_TYPE_REPLACE || op_type == OP_TYPE_MOVEREPLACE)
					&& index_shift[i].is_weakly_deleted()
				)
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
	if (op_type == OP_TYPE_MOVE || op_type == OP_TYPE_MOVEREPLACE) {
		for (ProgramCounterType i = 0; i < insert_tokens.size(); i++) {
			ProgramCounterType moved_out = old_src_pos + i;
			ProgramCounterType moved_in = new_dst_pos + i;
			index_shift[moved_out].index = moved_in;
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
		if (!index_shift[i].is_deleted() && index_shift[i].is_not_temp()) {
			offset++;
		}
		index_shift[i].op_priority = OP_PRIORITY_TEMP;
	}
	for (ProgramCounterType i = old_pos_end; i < index_shift.size(); i++) {
		if (index_shift[i].index > index_shift[old_pos_begin].index) {
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

void Program::move_tokens(ProgramCounterType old_begin, ProgramCounterType old_end, ProgramCounterType new_begin) {
	move_ops.push_back(MoveOp(old_begin, old_end, new_begin));
}

void Program::movereplace_tokens(
	ProgramCounterType old_begin, ProgramCounterType old_end,
	ProgramCounterType new_begin, ProgramCounterType new_end
) {
	movereplace_ops.push_back(MoveReplaceOp(old_begin, old_end, new_begin, new_end));
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
		OpPriority header_priority = op.priority;
		OpPriority remaining_priority = op.priority;
		if (op.priority == OP_PRIORITY_WEAK_DELETE) {
			header_priority = OP_PRIORITY_WEAK_DELETE;
			remaining_priority = OP_PRIORITY_STRONG_DELETE;
		}
		index_shift[op.pos_begin].op_priority = header_priority;
		for (ProgramCounterType token_i = op.pos_begin + 1; token_i < op.pos_end; token_i++) {
			index_shift[token_i].op_priority = remaining_priority;
		}
	}
	for (ProgramCounterType op_index = 0; op_index < insert_ops.size(); op_index++) {
		InsertOp& op = insert_ops[op_index];
		insert_op_exec(op.src_pos, op.dst_pos, op.insert_tokens, OP_TYPE_NORMAL);
	}
	for (MoveOp& op : move_ops | std::views::reverse) {
		if (index_shift[op.old_begin].op_priority > OP_PRIORITY_LIST_DELETE) {
			continue;
		}
		std::vector<Token> tokens_to_move(prev_tokens.begin() + op.old_begin, prev_tokens.begin() + op.old_end);
		delete_op_exec(op.old_begin, op.old_end, OP_TYPE_MOVE);
		insert_op_exec(op.old_begin, op.new_begin, tokens_to_move, OP_TYPE_MOVE);
		for (ProgramCounterType token_i = op.old_begin; token_i < op.old_end; token_i++) {
			index_shift[token_i].op_priority = OP_PRIORITY_MOVE;
		}
	}
	for (MoveReplaceOp& op : movereplace_ops | std::views::reverse) {
		IndexShiftEntry ise = index_shift[op.new_begin];
		delete_op_exec(op.old_begin, op.old_end, OP_TYPE_MOVE);
		if (ise.is_strongly_deleted() || ise.is_replaced()) {
			continue;
		}
		std::vector<Token> tokens_to_move(prev_tokens.begin() + op.old_begin, prev_tokens.begin() + op.old_end);
		delete_op_exec(op.new_begin, op.new_end, OP_TYPE_REPLACE);
		insert_op_exec(op.old_begin, op.new_begin, tokens_to_move, OP_TYPE_MOVEREPLACE);
		for (ProgramCounterType token_i = op.old_begin; token_i < op.old_end; token_i++) {
			index_shift[token_i].op_priority = OP_PRIORITY_MREP_SRC;
		}
		for (ProgramCounterType token_i = op.new_begin; token_i < op.new_end; token_i++) {
			index_shift[token_i].op_priority = OP_PRIORITY_REPLACE;
		}
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
		movereplace_ops.clear();
		new_pointers.clear();
		list_scope_stack = std::stack<ListScopeStackEntry>();
	} catch (std::exception exc) {
		throw std::runtime_error(__FUNCTION__": " + std::string(exc.what()));
	}
}

void Program::print_node(Node* node) {
	std::string indent_string = "";
	ProgramCounterType indent_level = node->get_parent_count(nodes);
	if (node->get_token(tokens)->is_list_end()) {
		indent_level--;
	}
	for (int j = 0; j < indent_level; j++) {
		indent_string += "    ";
	}
	std::cout << indent_string << node->get_token(tokens)->to_string() << "\n";
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
		if (result.is_ptr()) {
			new_pointers.insert(NewPointersEntry(program_counter, result.get_data_cast<PointerDataType>()));
		}
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