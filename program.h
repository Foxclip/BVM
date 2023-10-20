#pragma once

#include <vector>
#include <functional>
#include <string>
#include <stack>
#include <iostream>
#include <algorithm>
#include <ranges>
#include "token.h"
#include "utils.h"
#include <set>

const long long MAX_ITERATIONS = 10000000;

void throwUnexpectedCharException(char c, std::string current_word, ProgramCounterType line);

class Node {
public:
	PointerDataType parent_index = -1;
	ProgramCounterType arg_count = 0;
	std::vector<PointerDataType> arguments;
	ProgramCounterType first_index;
	ProgramCounterType last_index;

	Node();
	std::vector<Token> tokenize(std::vector<Node>& nodes, std::vector<Token>& tokens);
	Node* get_parent(std::vector<Node>& nodes);
	ProgramCounterType get_parent_count(std::vector<Node>& nodes);
	Token* get_token(std::vector<Token>& tokens);
};

class Program {
public:
	struct NewPointersEntry {
		PointerDataType index;
		PointerDataType pointer;
		NewPointersEntry(PointerDataType index, PointerDataType pointer);
	};
	std::vector<Token> tokens;
	std::vector<Token> prev_tokens;
	std::vector<Node> nodes;
	std::string local_print_buffer;
	std::string global_print_buffer;
	bool print_buffer_enabled = false;
	bool print_iterations = false;

	Program(std::string str);
	void print_tokens(std::vector<Token>& token_list, bool print_program_counter = true);
	void print_nodes();
	std::vector<Token> execute();

private:
	enum OpPriority {
		OP_PRIORITY_NULL,
		OP_PRIORITY_TEMP,
		OP_PRIORITY_FUNC_REPLACE,
		OP_PRIORITY_LIST_DELETE,
		OP_PRIORITY_MOVE,
		OP_PRIORITY_MREP_SRC,
		OP_PRIORITY_WEAK_DELETE,
		OP_PRIORITY_REPLACE,
		OP_PRIORITY_STRONG_DELETE,
	};
	class DeleteOp {
	public:
		ProgramCounterType pos_begin;
		ProgramCounterType pos_end;
		OpPriority priority;
		DeleteOp(ProgramCounterType pos_begin, ProgramCounterType pos_end, OpPriority priority);
	};
	class InsertOp {
	public:
		PointerDataType src_pos;
		ProgramCounterType dst_pos;
		std::vector<Token> insert_tokens;
		bool recalc_pointers = true;
		InsertOp(ProgramCounterType new_pos, std::vector<Token> insert_tokens);
		InsertOp(ProgramCounterType src_pos, ProgramCounterType dst_pos, std::vector<Token> insert_tokens);
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
	class MoveReplaceOp {
	public:
		ProgramCounterType old_begin;
		ProgramCounterType old_end;
		ProgramCounterType new_begin;
		ProgramCounterType new_end;
		MoveReplaceOp(
			ProgramCounterType old_begin, ProgramCounterType old_end,
			ProgramCounterType new_begin, ProgramCounterType new_end
		);
	};
	enum OpType {
		OP_TYPE_NORMAL,
		OP_TYPE_REPLACE,
		OP_TYPE_MOVE,
		OP_TYPE_MOVEREPLACE,
	};
	ProgramCounterType program_counter = 0;
	struct ListScopeStackEntry {
		ProgramCounterType pos;
		bool instruction_executed = false;
	};
	std::stack<ListScopeStackEntry> list_scope_stack;
	struct IndexShiftEntry {
		PointerDataType index = -1;
		OpPriority op_priority = OP_PRIORITY_NULL;
		bool is_deleted();
		bool is_list_deleted();
		bool is_weakly_deleted();
		bool is_strongly_deleted();
		bool is_normal_moved();
		bool is_moved();
		bool is_replaced();
		bool is_mrep_src();
		bool is_weakly_replaced();
		bool is_strongly_replaced();
		bool is_untouched();
		bool is_temp();
		bool is_not_temp();
	};
	std::vector<IndexShiftEntry> index_shift;
	std::vector<PointerDataType> index_shift_rev;
	std::vector<DeleteOp> delete_ops;
	std::vector<InsertOp> insert_ops;
	std::vector<ReplaceOp> replace_ops;
	std::vector<ReplaceOp> func_replace_ops;
	std::vector<MoveOp> move_ops;
	std::vector<MoveReplaceOp> movereplace_ops;
	std::set<NewPointersEntry> new_pointers;
	std::vector<Token> tokenize(std::string str);
	void parse();
	bool try_execute_instruction();
	PointerDataType token_index(std::vector<Token>& token_list, PointerDataType index);
	Token& get_token(std::vector<Token>& token_list, PointerDataType index);
	Token& rel_token(std::vector<Token>& token_list, PointerDataType offset);
	bool inside_seq();
	bool inside_list();
	bool parent_is_seq();
	bool parent_is_list();
	bool parent_is_if();
	PointerDataType to_dst_index(PointerDataType old_index);
	PointerDataType to_src_index(PointerDataType new_index);
	void insert_op_exec(PointerDataType old_src_pos, ProgramCounterType old_dst_pos, std::vector<Token> insert_tokens, OpType op_type);
	PointerDataType delete_op_exec(ProgramCounterType old_pos_begin, ProgramCounterType old_pos_end, OpType op_type);
	void delete_tokens(ProgramCounterType pos_begin, ProgramCounterType pos_end, OpPriority priority);
	void insert_tokens(ProgramCounterType old_pos, ProgramCounterType new_pos, std::vector<Token> insert_tokens);
	void replace_tokens(
		ProgramCounterType dst_begin, ProgramCounterType dst_end,
		ProgramCounterType src_begin, std::vector<Token> src_tokens
	);
	void replace_tokens_func(
		ProgramCounterType dst_begin, ProgramCounterType dst_end,
		ProgramCounterType src_begin, std::vector<Token> src_tokens
	);
	void move_tokens(ProgramCounterType old_begin, ProgramCounterType old_end, ProgramCounterType new_begin);
	void movereplace_tokens(
		ProgramCounterType old_begin, ProgramCounterType old_end,
		ProgramCounterType new_begin, ProgramCounterType new_end
	);
	void exec_replace_ops(std::vector<ReplaceOp>& vec, OpPriority priority);
	void exec_pending_ops();
	void reset_index_shift();
	void print_node(Node* node);
	void shift_pointers();
	void unary_func(std::function<Token(Token)> func);
	void binary_func(std::function<Token(Token, Token)> func);
	std::vector<Token> sys_call(int index, std::vector<Token> input);
	std::vector<Token> sys_print(std::vector<Token> input);

};

bool operator<(const Program::NewPointersEntry& left, const Program::NewPointersEntry& right);