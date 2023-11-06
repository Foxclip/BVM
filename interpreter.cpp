#include "interpreter.h"

Interpreter::NewPointersEntry::NewPointersEntry(PointerDataType index, PointerDataType pointer) {
	this->index = index;
	this->pointer = pointer;
}

Interpreter::DeleteOp::DeleteOp(ProgramCounterType pos_begin, ProgramCounterType pos_end, OpPriority priority) {
	this->pos_begin = pos_begin;
	this->pos_end = pos_end;
	this->priority = priority;
}

Interpreter::InsertOp::InsertOp(ProgramCounterType dst_pos, std::vector<Token> insert_tokens) {
	this->src_pos = -1;
	this->dst_pos = dst_pos;
	this->insert_tokens = insert_tokens;
	this->recalc_pointers = false;
}

Interpreter::InsertOp::InsertOp(ProgramCounterType src_pos, ProgramCounterType dst_pos, std::vector<Token> insert_tokens) {
	this->src_pos = src_pos;
	this->dst_pos = dst_pos;
	this->insert_tokens = insert_tokens;
	this->recalc_pointers = true;
}

Interpreter::ReplaceOp::ReplaceOp(
	ProgramCounterType dst_begin, ProgramCounterType dst_end,
	ProgramCounterType src_begin, std::vector<Token> src_tokens
) {
	this->dst_begin = dst_begin;
	this->dst_end = dst_end;
	this->src_begin = src_begin;
	this->src_tokens = src_tokens;
}

Interpreter::MoveOp::MoveOp(
	ProgramCounterType old_begin, ProgramCounterType old_end,
	ProgramCounterType new_begin
) {
	this->old_begin = old_begin;
	this->old_end = old_end;
	this->new_begin = new_begin;
}

Interpreter::MoveReplaceOp::MoveReplaceOp(
	ProgramCounterType old_begin, ProgramCounterType old_end,
	ProgramCounterType new_begin, ProgramCounterType new_end
) {
	this->old_begin = old_begin;
	this->old_end = old_end;
	this->new_begin = new_begin;
	this->new_end = new_end;
}

Interpreter::Interpreter(std::string str) {
	tokens = Compiler().compile(str);
}

void Interpreter::print_tokens(std::vector<Token>& token_list, bool print_program_counter) {
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

void Interpreter::print_nodes() {
	parse(0, false);
	for (ProgramCounterType i = 0; i < tokens.size(); i++) {
		print_node(tokens[i]);
	}
}

std::vector<Token> Interpreter::execute() {
	try {
		prev_tokens = tokens;
		if (print_iterations) {
			std::cout << "Iteration *: ";
			print_tokens(tokens, false);
		}
		for (ProgramCounterType iteration = 0; iteration < max_iterations; iteration++) {
			if (print_iterations) {
				std::cout << "Iteration " << iteration << ": ";
			}
			ProgramCounterType steps = 0;
			reset_index_shift();
			local_print_buffer = "";
			parse(0, false);
			prev_tokens = tokens;
			auto jump_to_end = [&]() {
				Token& seq_node = prev_tokens[scope_list.back().pos];
				program_counter = seq_node.last_index;
			};
			auto notify_parents = [&]() {
				for (int i = 0; i < scope_list.size(); i++) {
					scope_list[i].instruction_executed = true;
				}
			};
			auto exit_parent = [&]() {
				jump_to_end();
				scope_list.pop_back();
				notify_parents();
			};
			auto try_exec_normal = [&]() {
				try_execute_func_instruction();
				notify_parents();
			};
			auto try_exec_silent = [&]() {
				try_execute_func_instruction();
			};
			for (program_counter = 0; program_counter < prev_tokens.size(); program_counter++) {
				Token& current_token = prev_tokens[program_counter];
				if (current_token.is_num_or_ptr()) {
					// skipping
				} else if (parent_is_seq_or_useq() && scope_list.back().instruction_executed) {
					exit_parent();
				} else if (current_token.is_container_header()) {
					scope_list.push_back({ program_counter, false });
					try_exec_silent();
				} else if (current_token.str == "end") {
					if (parent_is_ulist_or_useq()) {
						try_exec_normal();
					} else {
						try_exec_silent();
					}
					scope_list.pop_back();
				} else if (current_token.str == "q") {
					try_exec_silent();
				} else {
					if (parent_is_container(program_counter, false)) {
			   			try_exec_normal();
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
			global_print_buffer += local_print_buffer;
			if (print_buffer_enabled && local_print_buffer.size() > 0) {
				if (print_iterations) {
					std::cout << "Print: ";
				}
				std::cout << local_print_buffer;
				if (print_iterations && !utils::is_newline(local_print_buffer.back())) {
					std::cout << "\n";
				}
			}
			if (tokens == prev_tokens) {
				break;
			}
		}
		return tokens;
	} catch (std::exception exc) {
		throw std::runtime_error(__FUNCTION__": " + std::string(exc.what()));
	}
}

bool Interpreter::try_execute_func_instruction() {
	Token current_token = rel_token(prev_tokens, 0);
	if (current_token.str == "add") {
		return binary_func([](Token a, Token b) { return Token::add(a, b); });
	} else if (current_token.str == "sub") {
		return binary_func([](Token a, Token b) { return Token::sub(a, b); });
	} else if (current_token.str == "mul") {
		return binary_func([](Token a, Token b) { return Token::mul(a, b); });
	} else if (current_token.str == "div") {
		return binary_func([](Token a, Token b) { return Token::div(a, b); });
	} else if (current_token.str == "mod") {
		return binary_func([](Token a, Token b) { return Token::mod(a, b); });
	} else if (current_token.str == "pow") {
		return binary_func([](Token a, Token b) { return Token::pow(a, b); });
	} else if (current_token.str == "log") {
		return unary_func([](Token a) { return Token::log(a); });
	} else if (current_token.str == "log2") {
		return unary_func([](Token a) { return Token::log2(a); });
	} else if (current_token.str == "sin") {
		return unary_func([](Token a) { return Token::sin(a); });
	} else if (current_token.str == "cos") {
		return unary_func([](Token a) { return Token::cos(a); });
	} else if (current_token.str == "tan") {
		return unary_func([](Token a) { return Token::tan(a); });
	} else if (current_token.str == "asin") {
		return unary_func([](Token a) { return Token::asin(a); });
	} else if (current_token.str == "acos") {
		return unary_func([](Token a) { return Token::acos(a); });
	} else if (current_token.str == "atan") {
		return unary_func([](Token a) { return Token::atan(a); });
	} else if (current_token.str == "atan2") {
		return binary_func([](Token a, Token b) { return Token::atan2(a, b); });
	} else if (current_token.str == "floor") {
		return unary_func([](Token a) { return Token::floor(a); });
	} else if (current_token.str == "ceil") {
		return unary_func([](Token a) { return Token::ceil(a); });
	} else if (current_token.str == "cmp") {
		return binary_func([](Token a, Token b) { return Token::cmp(a, b); });
	} else if (current_token.str == "lt") {
		return binary_func([](Token a, Token b) { return Token::lt(a, b); });
	} else if (current_token.str == "gt") {
		return binary_func([](Token a, Token b) { return Token::gt(a, b); });
	} else if (current_token.str == "and") {
		return binary_func([](Token a, Token b) { return Token::and_op(a, b); });
	} else if (current_token.str == "or") {
		return binary_func([](Token a, Token b) { return Token::or_op(a, b); });
	} else if (current_token.str == "xor") {
		return binary_func([](Token a, Token b) { return Token::xor_op(a, b); });
	} else if (current_token.str == "not") {
		return unary_func([](Token a) { return Token::not_op(a); });
	} else {
		return try_execute_mod_instruction();
	}
}

bool Interpreter::try_execute_mod_instruction() {
	Token current_token = rel_token(prev_tokens, 0);
	if (current_token.str == "cpy") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr() && rel_token(prev_tokens, 2).is_num_or_ptr()) {
			PointerDataType src = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			PointerDataType dst = rel_token(prev_tokens, 2).get_data_cast<PointerDataType>();
			ProgramCounterType src_index_begin = token_index(prev_tokens, program_counter + 1 + src);
			ProgramCounterType dst_index = token_index(prev_tokens, program_counter + 2 + dst);
			delete_tokens(program_counter, program_counter + 3, OP_PRIORITY_WEAK_DELETE);
			if (src_index_begin != prev_tokens.size() && prev_tokens[src_index_begin].str != "end" && parent_is_container(dst_index, true)) {
				Token* node = &prev_tokens[token_index(prev_tokens, src_index_begin)];
				std::vector<Token> node_tokens = node->tokenize(prev_tokens);
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
			if (target_index != prev_tokens.size() && prev_tokens[target_index].str != "end" && parent_is_container(target_index, true)) {
				Token* node = &prev_tokens[token_index(prev_tokens, target_index)];
				std::vector<Token> node_tokens = node->tokenize(prev_tokens);
				delete_tokens(target_index, target_index + node_tokens.size(), OP_PRIORITY_STRONG_DELETE);
			}
			return true;
		}
		return false;
	} else if (current_token.str == "get") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr()) {
			PointerDataType src = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			PointerDataType src_index_begin = token_index(prev_tokens, program_counter + 1 + src);
			if (src_index_begin != prev_tokens.size() && prev_tokens[src_index_begin].str != "end") {
				Token* src_node = &prev_tokens[token_index(prev_tokens, src_index_begin)];
				std::vector<Token> src_node_tokens = src_node->tokenize(prev_tokens);
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
			delete_tokens(program_counter, prev_tokens[program_counter].last_index + 1, OP_PRIORITY_WEAK_DELETE);
			if (dst_index_begin != prev_tokens.size() && prev_tokens[dst_index_begin].str != "end") {
				Token* src_node = &prev_tokens[token_index(prev_tokens, src_index_begin)];
				if (prev_tokens[src_node->first_index].str == "q") {
					src_node = &prev_tokens[src_node->arguments[0]];
				}
				Token* dst_node = &prev_tokens[token_index(prev_tokens, dst_index_begin)];
				std::vector<Token> src_node_tokens = src_node->tokenize(prev_tokens);
				std::vector<Token> dst_node_tokens = dst_node->tokenize(prev_tokens);
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
			delete_tokens(program_counter, prev_tokens[program_counter].last_index + 1, OP_PRIORITY_WEAK_DELETE);
			if (parent_is_container(dst_index_begin, true)) {
				Token* src_node = &prev_tokens[token_index(prev_tokens, src_index_begin)];
				if (prev_tokens[src_node->first_index].str == "q") {
					src_node = &prev_tokens[src_node->arguments[0]];
				}
				std::vector<Token> src_node_tokens = src_node->tokenize(prev_tokens);
				insert_tokens(src_index_begin, dst_index_begin, src_node_tokens);
			}
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
			if (
				src_index_begin != prev_tokens.size() && dst_index_begin != prev_tokens.size()
				&& prev_tokens[src_index_begin].str != "end" && prev_tokens[dst_index_begin].str != "end"
			) {
				Token* dst_node = &prev_tokens[token_index(prev_tokens, dst_index_begin)];
				Token* src_node = &prev_tokens[token_index(prev_tokens, src_index_begin)];
				ProgramCounterType dst_index_end = dst_node->last_index + 1;
				std::vector<Token> src_node_tokens = src_node->tokenize(prev_tokens);
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
			if (src_index_begin != prev_tokens.size() && parent_is_container(src_index_begin, true) && parent_is_container(dst_index_begin, true)) {
				Token& src_token = prev_tokens[src_index_begin];
				if (src_token.str == "end") {
					RangePair move_range = get_end_move_range(prev_tokens, src_index_begin);
					dst_index_begin = std::clamp(dst_index_begin, move_range.first, move_range.last);
				}
				move_tokens(src_index_begin, src_token.last_index + 1, dst_index_begin);
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
			Token& src_token = prev_tokens[src_index_begin];
			bool within_move_range = true;
			if (src_token.str == "end") {
				RangePair move_range = get_end_move_range(prev_tokens, src_index_begin);
				within_move_range = dst_index_begin >= move_range.first && dst_index_begin <= move_range.last;
			}
			if (
				src_index_begin != prev_tokens.size()
				&& dst_index_begin != prev_tokens.size()
				&& prev_tokens[dst_index_begin].str != "end"
				&& within_move_range
				&& parent_is_container(src_index_begin, true)
			) {
				Token* src_node = &prev_tokens[token_index(prev_tokens, src_index_begin)];
				Token* dst_node = &prev_tokens[token_index(prev_tokens, dst_index_begin)];
				movereplace_tokens(
					src_index_begin, src_node->last_index + 1,
					dst_index_begin, dst_node->last_index + 1
				);
			}
			return true;
		}
		return false;
	} else if (current_token.str == "if") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr()) {
			BoolType cond = rel_token(prev_tokens, 1).get_data_cast<BoolType>();
			Token* if_node = &prev_tokens[token_index(prev_tokens, program_counter)];
			Token* true_node = &prev_tokens[if_node->arguments[1]];
			Token* false_node = &prev_tokens[if_node->arguments[2]];
			Token* selected_node = cond != 0 ? true_node : false_node;
			if (prev_tokens[selected_node->first_index].str == "q") {
				selected_node = &prev_tokens[selected_node->arguments[0]];
			}
			movereplace_tokens(
				selected_node->first_index, selected_node->last_index + 1,
				if_node->first_index, if_node->last_index + 1
			);
			return true;
		}
		return false;
	} else if (current_token.str == "cast") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr() && rel_token(prev_tokens, 2).is_num_or_ptr()) {
			Token& arg1 = rel_token(prev_tokens, 1);
			Token& arg2 = rel_token(prev_tokens, 2);
			if (arg1.is_ptr()) {
				arg1.set_data<PointerDataType>(arg1.get_data<PointerDataType>() + 1);
			}
			if (arg2.is_ptr()) {
				arg2.set_data<PointerDataType>(arg2.get_data<PointerDataType>() + 2);
			}
			token_type type = static_cast<token_type>(arg1.get_data_cast<Int32Type>());
			if (type >= 0 && type < type_unknown && type != type_instr) {
				arg2.cast(type);
				if (type == type_ptr) {
					arg2.set_data<PointerDataType>(arg2.get_data<PointerDataType>() + 2);
				}
			}
			Token result = arg2;
			result.str = result.to_string();
			if (result.is_ptr()) {
				new_pointers.insert(NewPointersEntry(program_counter, result.get_data_cast<PointerDataType>()));
			}
			replace_tokens_func(program_counter, program_counter + 3, program_counter, { result });
			return true;
		}
		return false;
	} else if (current_token.str == "print") {
		if (rel_token(prev_tokens, 1).str == "list") {
			ProgramCounterType char_token_index = 1;
			std::string str;
			while (true) {
				char_token_index++;
				Token& char_token = rel_token(prev_tokens, char_token_index);
				if (char_token.str == "end") {
					break;
				}
				char c = char_token.get_data_cast<Int32Type>();
				str += c;
			}
			local_print_buffer += str;
			delete_tokens(program_counter, program_counter + char_token_index + 1, OP_PRIORITY_WEAK_DELETE);
			return true;
		} else if (rel_token(prev_tokens, 1).is_num_or_ptr()) {
			Token& char_token = rel_token(prev_tokens, 1);
			char c = char_token.get_data_cast<Int32Type>();
			local_print_buffer += c;
			delete_tokens(program_counter, program_counter + 2, OP_PRIORITY_WEAK_DELETE);
			return true;
		}
		return false;
	} else if (current_token.str == "str") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr()) {
			Token& arg = rel_token(prev_tokens, 1);
			std::string str = arg.to_string();
			std::vector<Token> results;
			results.push_back(Token("list"));
			for (ProgramCounterType char_i = 0; char_i < str.size(); char_i++) {
				Token char_token;
				char_token.type = type_int32;
				char_token.set_data<Int32Type>(str[char_i]);
				results.push_back(char_token);
			}
			results.push_back(Token("end"));
			replace_tokens_func(program_counter, program_counter + 2, program_counter, results);
			return true;
		}
		return false;
	} else if (current_token.str == "box") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr() && rel_token(prev_tokens, 2).is_num_or_ptr()) {
			PointerDataType begin = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			PointerDataType end = rel_token(prev_tokens, 2).get_data_cast<PointerDataType>();
			ProgramCounterType begin_index = token_index(prev_tokens, program_counter + 1 + begin);
			ProgramCounterType end_index = token_index(prev_tokens, program_counter + 2 + end);
			ProgramCounterType begin_index_new = std::min(begin_index, end_index);
			ProgramCounterType end_index_new = std::max(begin_index, end_index) + 1;
			delete_tokens(program_counter, program_counter + 3, OP_PRIORITY_WEAK_DELETE);
			bool same_parent = prev_tokens[begin_index_new].parent_index == prev_tokens[end_index_new - 1].parent_index;
			bool cont_args = same_parent && parent_is_container(begin_index_new, true);
			bool one_arg = prev_tokens[begin_index_new].last_index == end_index_new - 1;
			if (cont_args || one_arg) {
				insert_tokens(0, begin_index_new, { Token("list") });
				insert_tokens(0, end_index_new, { Token("end") });
			}
			return true;
		}
		return false;
	} else if (current_token.str == "unbox") {
		if (rel_token(prev_tokens, 1).is_num_or_ptr()) {
			PointerDataType arg = rel_token(prev_tokens, 1).get_data_cast<PointerDataType>();
			ProgramCounterType header_index = token_index(prev_tokens, program_counter + 1 + arg);
			delete_tokens(program_counter, program_counter + 2, OP_PRIORITY_WEAK_DELETE);
			bool one_arg = prev_tokens[header_index].arguments.size() == 2;
			if (prev_tokens[header_index].is_container_header() && (parent_is_container(header_index, true) || one_arg)) {
				ProgramCounterType end_index = prev_tokens[header_index].last_index;
				delete_tokens(header_index, header_index + 1, OP_PRIORITY_STRONG_DELETE);
				delete_tokens(end_index, end_index + 1, OP_PRIORITY_STRONG_DELETE);
			}
			return true;
		}
		return false;
	} else if (current_token.str == "list") {
		return true;
	} else if (current_token.str == "seq") {
		return true;
	} else if (current_token.str == "ulist") {
		return true;
	} else if (current_token.str == "useq") {
		return true;
	} else if (current_token.str == "end") {
		PointerDataType header_index = prev_tokens[program_counter].parent_index;
		auto one_arg = [&]() { return prev_tokens[header_index].arguments.size() == 2; };
		if (
			parent_is_ulist_or_useq()
			&& !scope_list.back().instruction_executed
			&& (parent_is_container(header_index, true) || one_arg())
		) {
			delete_tokens(header_index, header_index + 1, OP_PRIORITY_LIST_DELETE);
			delete_tokens(program_counter, program_counter + 1, OP_PRIORITY_LIST_DELETE);
		}
		return true;
	} else if (current_token.str == "q") {
		Token& node = prev_tokens[program_counter];
		program_counter = node.last_index;
		return true;
	} else {
		throw std::runtime_error("Unexpected token: " + current_token.str);
	}
}

void Interpreter::parse(ProgramCounterType index, bool one) {
	try {
		std::stack<PointerDataType> parent_stack;
		for (PointerDataType token_i = index; token_i < tokens.size(); token_i++) {
			Token& current_token = tokens[token_i];
			if (current_token.str == "end" && parent_stack.empty()) {
				throw std::runtime_error("Mismathed end");
			}
			current_token.parent_index = -1;
			current_token.arg_count = 0;
			current_token.arguments.clear(); // might reinitialize instead for saving memory
			current_token.first_index = token_i;
			current_token.last_index = 0;
			if (!parent_stack.empty()) {
				current_token.parent_index = parent_stack.top();
				tokens[parent_stack.top()].arguments.push_back(token_i);
			}
			ProgramCounterType arg_count = 0;
			if (!current_token.is_num_or_ptr()) {
				arg_count = get_arg_count(current_token.get_data_cast<InstructionDataType>());
			}
			current_token.arg_count = arg_count;
			if (arg_count > 0) {
				parent_stack.push(token_i);
			} else {
				current_token.last_index = token_i;
			}
			ProgramCounterType current_index = token_i;
			ProgramCounterType current_last_index;
			bool first = true;
			while (!parent_stack.empty()) {
				Token& current_parent = tokens[parent_stack.top()];
				ProgramCounterType arg_offset = current_index - current_parent.first_index;
				bool arg_offset_end = arg_offset >= current_parent.arg_count;
				bool end_end = tokens[current_index].str == "end";
				auto exit_level = [&]() {
					if (first) {
						current_last_index = current_index;
						first = false;
					}
					tokens[parent_stack.top()].last_index = current_last_index;
					current_index = tokens[parent_stack.top()].first_index;
					parent_stack.pop();
				};
				if (arg_offset_end || end_end) {
					exit_level();
				} else {
					break;
				}
			}
			if (one && parent_stack.empty()) {
				break;
			}
		}
		if (!parent_stack.empty()) {
			throw std::runtime_error("Missing end");
		}
	} catch (std::exception exc) {
		throw std::runtime_error(__FUNCTION__": " + std::string(exc.what()));
	}
}

PointerDataType Interpreter::token_index(std::vector<Token>& token_list, PointerDataType index) {
	return utils::mod(index, (PointerDataType)token_list.size() + 1);
}

Token& Interpreter::get_token(std::vector<Token>& token_list, PointerDataType index) {
	return token_list[token_index(token_list, index)];
}

Token& Interpreter::rel_token(std::vector<Token>& token_list, PointerDataType offset) {
	return get_token(token_list, program_counter + offset);
}

bool Interpreter::inside_seq() {
	return
		scope_list.size() > 0
		&& get_token(prev_tokens, scope_list.back().pos).str == "seq"
	;
}

bool Interpreter::inside_list() {
	return
		scope_list.size() > 0
		&& get_token(prev_tokens, scope_list.back().pos).str == "list"
	;
}

bool Interpreter::inside_container() {
	return false;
}

bool Interpreter::parent_is_seq_or_useq() {
	PointerDataType parent_index = prev_tokens[program_counter].parent_index;
	return parent_index >= 0
		&& (prev_tokens[parent_index].str == "seq" || prev_tokens[parent_index].str == "useq");
}

bool Interpreter::parent_is_list_or_ulist() {
	PointerDataType parent_index = prev_tokens[program_counter].parent_index;
	return parent_index >= 0
		&& (prev_tokens[parent_index].str == "list" || prev_tokens[parent_index].str == "ulist");
}

bool Interpreter::parent_is_ulist_or_useq() {
	PointerDataType parent_index = prev_tokens[program_counter].parent_index;
	return parent_index >= 0
		&& (prev_tokens[parent_index].str == "ulist" || prev_tokens[parent_index].str == "useq");
}

bool Interpreter::parent_is_if() {
	PointerDataType parent_index = prev_tokens[program_counter].parent_index;
	return parent_index >= 0 && prev_tokens[parent_index].str == "if";
}

bool Interpreter::IndexShiftEntry::is_deleted() {
	return
		op_priority == OP_PRIORITY_WEAK_DELETE
		|| op_priority == OP_PRIORITY_STRONG_DELETE
	;
}

bool Interpreter::IndexShiftEntry::is_weakly_deleted() {
	return op_priority == OP_PRIORITY_WEAK_DELETE;
}

bool Interpreter::IndexShiftEntry::is_strongly_deleted() {
	return op_priority == OP_PRIORITY_STRONG_DELETE;
}

bool Interpreter::IndexShiftEntry::is_moved() {
	return is_normal_moved() || is_mrep_src();
}

bool Interpreter::IndexShiftEntry::is_normal_moved() {
	return op_priority == OP_PRIORITY_MOVE;
}

bool Interpreter::IndexShiftEntry::is_replaced() {
	return 
		op_priority == OP_PRIORITY_REPLACE 
		|| op_priority == OP_PRIORITY_FUNC_REPLACE
	;
}

bool Interpreter::IndexShiftEntry::is_mrep_src() {
	return op_priority == OP_PRIORITY_MREP_SRC;
}

bool Interpreter::IndexShiftEntry::is_weakly_replaced() {
	return op_priority == OP_PRIORITY_FUNC_REPLACE;
}

bool Interpreter::IndexShiftEntry::is_strongly_replaced() {
	return op_priority == OP_PRIORITY_REPLACE;
}

bool Interpreter::IndexShiftEntry::is_untouched() {
	return op_priority == OP_PRIORITY_NULL;
}

bool Interpreter::IndexShiftEntry::is_temp() {
	return op_priority == OP_PRIORITY_TEMP;
}

bool Interpreter::IndexShiftEntry::is_not_temp() {
	return op_priority != OP_PRIORITY_TEMP;
}

PointerDataType Interpreter::to_dst_index(PointerDataType old_index) {
	return index_shift[old_index].index;
}

PointerDataType Interpreter::to_src_index(PointerDataType new_index) {
	return index_shift_rev[new_index];
}

void Interpreter::insert_op_exec(PointerDataType old_src_pos, ProgramCounterType old_dst_pos, std::vector<Token> insert_tokens, OpType op_type) {
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

PointerDataType Interpreter::delete_op_exec(ProgramCounterType old_pos_begin, ProgramCounterType old_pos_end, OpType op_type) {
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

bool Interpreter::parent_is_container(ProgramCounterType index, bool root_is_container) {
	auto end = [&]() { return index == prev_tokens.size(); };
	auto hasparent = [&]() { return prev_tokens[index].has_parent(); };
	auto parent_is_cont = [&]() { return prev_tokens[index].has_parent() && prev_tokens[index].get_parent(prev_tokens).is_container_header(); };
	bool result;
	if (root_is_container) {
		result = end() || !hasparent() || parent_is_cont();
	} else {
		result = parent_is_cont();
	}
	return result;
}

PointerDataType Interpreter::next_arg_parent(ProgramCounterType index) {
	Token* current_token = &prev_tokens[index];
	while (current_token->last_index < index + 1) {
		if (!current_token->has_parent()) {
			return -1;
		}
		current_token = &current_token->get_parent(prev_tokens);
	}
	return current_token->first_index;
}

Interpreter::RangePair Interpreter::get_end_move_range(std::vector<Token>& tokens, ProgramCounterType index) {
	Token& token = tokens[index];
	ProgramCounterType clamp_begin = token.parent_index + 1;
	ProgramCounterType clamp_end = -1;
	Token& header_token = token.get_parent(prev_tokens);
	if (header_token.has_parent()) {
		clamp_end = header_token.get_parent(prev_tokens).last_index;
	}
	return { clamp_begin, clamp_end };
}

void Interpreter::delete_tokens(ProgramCounterType pos_begin, ProgramCounterType pos_end, OpPriority priority) {
	delete_ops.push_back(DeleteOp(pos_begin, pos_end, priority));
}

void Interpreter::insert_tokens(ProgramCounterType old_pos, ProgramCounterType new_pos, std::vector<Token> insert_tokens) {
	insert_ops.push_back(InsertOp(old_pos, new_pos, insert_tokens));
}

void Interpreter::replace_tokens(
	ProgramCounterType dst_begin, ProgramCounterType dst_end,
	ProgramCounterType src_begin, std::vector<Token> src_tokens
) {
	replace_ops.push_back(ReplaceOp(dst_begin, dst_end, src_begin, src_tokens));
}

void Interpreter::replace_tokens_func(
	ProgramCounterType dst_begin, ProgramCounterType dst_end,
	ProgramCounterType src_begin, std::vector<Token> src_tokens
) {
	func_replace_ops.push_back(ReplaceOp(dst_begin, dst_end, src_begin, src_tokens));
}

void Interpreter::move_tokens(ProgramCounterType old_begin, ProgramCounterType old_end, ProgramCounterType new_begin) {
	move_ops.push_back(MoveOp(old_begin, old_end, new_begin));
}

void Interpreter::movereplace_tokens(
	ProgramCounterType old_begin, ProgramCounterType old_end,
	ProgramCounterType new_begin, ProgramCounterType new_end
) {
	movereplace_ops.push_back(MoveReplaceOp(old_begin, old_end, new_begin, new_end));
}

void Interpreter::exec_replace_ops(std::vector<ReplaceOp>& vec, OpPriority priority) {
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

void Interpreter::exec_pending_ops() {
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
		if (index_shift[op.old_begin].op_priority >= OP_PRIORITY_MOVE) {
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

void Interpreter::reset_index_shift() {
		index_shift = std::vector<IndexShiftEntry>(tokens.size() + 1);
		index_shift_rev = std::vector<PointerDataType>(tokens.size() + 1);
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
		scope_list = std::vector<ScopeListEntry>();
}

void Interpreter::print_node(Token& token) {
	std::string indent_string = "";
	ProgramCounterType indent_level = token.get_parent_count(tokens);
	if (token.str == "end") {
		indent_level--;
	}
	for (int j = 0; j < indent_level; j++) {
		indent_string += "    ";
	}
	std::cout << indent_string << token.orig_str << "\n";
}

void Interpreter::shift_pointers() {
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

bool Interpreter::unary_func(std::function<Token(Token)> func) {
	if (rel_token(prev_tokens, 1).is_num_or_ptr()) {
		Token& arg = rel_token(prev_tokens, 1);
		if (arg.is_ptr()) {
			arg.set_data<PointerDataType>(arg.get_data<PointerDataType>() + 1);
		}
		Token result = func(arg);
		if (result.is_ptr()) {
			new_pointers.insert(NewPointersEntry(program_counter, result.get_data_cast<PointerDataType>()));
		}
		replace_tokens_func(program_counter, program_counter + 2, program_counter, { result });
		return true;
	}
	return false;
}

bool Interpreter::binary_func(std::function<Token(Token, Token)> func) {
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
		return true;
	}
	return false;
}

bool operator<(const Interpreter::NewPointersEntry& left, const Interpreter::NewPointersEntry& right) {
	return left.index < right.index;
}