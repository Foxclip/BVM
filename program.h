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
	class DeleteOp {
	public:
		ProgramCounterType pos_begin;
		ProgramCounterType pos_end;
		DeleteOp(ProgramCounterType pos_begin, ProgramCounterType pos_end);
	};
	class InsertOp {
	public:
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
	class MoveOp {
	public:
		ProgramCounterType old_begin;
		ProgramCounterType old_end;
		ProgramCounterType new_begin;
		MoveOp(
			ProgramCounterType old_begin, ProgramCounterType old_end,
			ProgramCounterType new_begin
		);
	};
	ProgramCounterType program_counter = 0;
	struct IndexShiftEntry {
		PointerDataType index = -1;
		bool deleted = false;
	};
	std::vector<IndexShiftEntry> index_shift;
	std::vector<PointerDataType> index_shift_rev;
	std::vector<DeleteOp> delete_ops;
	std::vector<InsertOp> insert_ops;
	std::vector<ReplaceOp> replace_ops;
	std::vector<MoveOp> move_ops;

	std::vector<Token> tokenize(std::string str);
	void parse();
	PointerDataType token_index(std::vector<Token>& token_list, PointerDataType index);
	Token& get_token(std::vector<Token>& token_list, PointerDataType index);
	Token& rel_token(std::vector<Token>& token_list, PointerDataType offset);
	std::unique_ptr<Node> parse_token(
		std::vector<Token>& token_list, PointerDataType token_index,
		Node* parent_node, PointerDataType& new_token_index, int depth = 0
	);
	bool is_deleted(ProgramCounterType old_index);
	PointerDataType to_dst_index(PointerDataType old_index);
	PointerDataType to_src_index(PointerDataType new_index);
	void insert_op_exec(ProgramCounterType old_pos, std::vector<Token> insert_tokens);
	void delete_op_exec(ProgramCounterType old_pos_begin, ProgramCounterType old_pos_end, bool p_delete = true);
	void delete_tokens(ProgramCounterType pos_begin, ProgramCounterType pos_end);
	void insert_tokens(ProgramCounterType old_pos, ProgramCounterType new_pos, std::vector<Token> insert_tokens);
	void replace_tokens(
		ProgramCounterType dst_begin, ProgramCounterType dst_end,
		ProgramCounterType src_begin, std::vector<Token> src_tokens
	);
	void exec_pending_ops();
	void reset_index_shift();
	void print_node(Node* node, int indent_level);
	void shift_pointers();
	void unary_func(std::function<Token(Token)> func);
	void binary_func(std::function<Token(Token, Token)> func);
	std::vector<Token> sys_call(int index, std::vector<Token> input);
	std::vector<Token> sys_print(std::vector<Token> input);

};