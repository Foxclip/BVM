#pragma once

#include <vector>
#include <functional>
#include <string>
#include <stack>
#include <iostream>
#include <algorithm>
#include "token.h"
#include "utils.h"

const long long MAX_ITERATIONS = 1000;
const long long MAX_NESTED_NODES = 100;

void throwUnexpectedCharException(char c, std::string current_word, ProgramCounterType line);

class Node {
public:
	Token token;
	std::vector<std::unique_ptr<Node>> arguments;

	Node(Token token);
	std::string to_string();
	std::vector<Token> tokenize();
};

class Program {
public:
	std::vector<Token> tokens;
	std::vector<Token> prev_tokens;
	std::vector<std::unique_ptr<Node>> nodes;
	std::string local_print_buffer;
	std::string global_print_buffer;
	bool print_buffer_enabled = false;
	bool print_iterations = false;

	Program(std::string str);
	void print_tokens(std::vector<Token>& token_list, bool print_program_counter = true);
	void print_nodes();
	std::vector<Token> execute();

private:
	class ModifyOp {
	public:
		ProgramCounterType new_pos;
		ProgramCounterType old_pos;
		Token new_token;
		bool recalc_pointers = true;
		ModifyOp(ProgramCounterType pos, Token new_token);
		ModifyOp(ProgramCounterType old_pos, ProgramCounterType new_pos, Token new_token);
	};
	class DeleteOp {
	public:
		ProgramCounterType pos_begin;
		ProgramCounterType pos_end;
		DeleteOp(ProgramCounterType pos_begin, ProgramCounterType pos_end);
	};
	class InsertOp {
	public:
		ProgramCounterType old_pos;
		ProgramCounterType new_pos;
		std::vector<Token> insert_tokens;
		bool recalc_pointers = true;
		InsertOp(ProgramCounterType new_pos, std::vector<Token> insert_tokens);
		InsertOp(ProgramCounterType old_pos, ProgramCounterType new_pos, std::vector<Token> insert_tokens);
	};
	class ReplaceOp {
	public:
		ProgramCounterType dst_begin;
		ProgramCounterType dst_end;
		ProgramCounterType src_begin;
		std::vector<Token> src_tokens;
		ReplaceOp(
			ProgramCounterType dst_begin, ProgramCounterType dst_end,
			ProgramCounterType src_begin, std::vector<Token> src_tokens
		);
	};
	ProgramCounterType program_counter = 0;
	std::vector<PointerDataType> index_shift;
	std::vector<ModifyOp> modify_ops;
	std::vector<DeleteOp> delete_ops;
	std::vector<InsertOp> insert_ops;
	std::vector<ReplaceOp> replace_ops;

	std::vector<Token> tokenize(std::string str);
	void parse();
	PointerDataType token_index(std::vector<Token>& token_list, PointerDataType index);
	Token& get_token(std::vector<Token>& token_list, PointerDataType index);
	Token& rel_token(std::vector<Token>& token_list, PointerDataType offset);
	std::unique_ptr<Node> parse_token(
		std::vector<Token>& token_list, PointerDataType token_index,
		Node* parent_node, PointerDataType& new_token_index, int depth = 0
	);
	PointerDataType to_dst_index(ProgramCounterType old_index);
	void shift_indices(ProgramCounterType pos, PointerDataType offset);
	void reset_index_shift();
	void modify_token(ProgramCounterType pos, Token new_token);
	void delete_tokens(ProgramCounterType pos_begin, ProgramCounterType pos_end);
	void insert_tokens(ProgramCounterType old_pos, ProgramCounterType new_pos, std::vector<Token> insert_tokens);
	void replace_tokens(
		ProgramCounterType dst_begin, ProgramCounterType dst_end,
		ProgramCounterType src_begin, std::vector<Token> src_tokens
	);
	void delete_op_exec(ProgramCounterType pos_begin, ProgramCounterType pos_end);
	void insert_op_exec(
		ProgramCounterType old_pos, ProgramCounterType new_pos,
		std::vector<Token> insert_tokens, bool recalc_pointers
	);
	void exec_pending_ops();
	void print_node(Node* node, int indent_level);
	PointerDataType recalc_pointer(
		std::vector<Token>& parent_token_list,
		PointerDataType pointer_index, PointerDataType pointer,
		PointerDataType pos, PointerDataType offset,
		bool pin_index, bool pin_dst
	);
	void _shift_pointers(
		std::vector<Token>& token_list, std::vector<Token>& parent_token_list,
		PointerDataType list_pos, PointerDataType pos, PointerDataType offset,
		bool pin_index, bool pin_dst
	);
	void shift_pointers(
		std::vector<Token>& token_list,
		PointerDataType pos, PointerDataType offset
	);
	void shift_pointers_rel(
		std::vector<Token>& token_list, std::vector<Token>& parent_token_list,
		ProgramCounterType list_pos, PointerDataType pos, PointerDataType offset,
		bool pin_index, bool pin_dst
	);
	bool unary_func(std::function<Token(Token)> func);
	void binary_func(std::function<Token(Token, Token)> func);
	std::vector<Token> sys_call(int index, std::vector<Token> input);
	std::vector<Token> sys_print(std::vector<Token> input);

};