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
	struct DeleteOp {
		ProgramCounterType pos;
		ProgramCounterType count;
	};
	struct InsertOp {
		ProgramCounterType pos;
		std::vector<Token> insert_tokens;
	};
	ProgramCounterType program_counter = 0;
	std::vector<ProgramCounterType> index_shift;
	std::vector<DeleteOp> delete_ops;
	std::vector<InsertOp> insert_ops;

	std::vector<Token> tokenize(std::string str);
	void parse();
	PointerDataType token_index(std::vector<Token>& token_list, PointerDataType index);
	Token& get_token(std::vector<Token>& token_list, PointerDataType index);
	Token& rel_token(std::vector<Token>& token_list, PointerDataType offset);
	std::unique_ptr<Node> parse_token(
		std::vector<Token>& token_list, PointerDataType token_index,
		Node* parent_node, PointerDataType& new_token_index, int depth = 0
	);
	ProgramCounterType to_dst_index(ProgramCounterType old_index);
	void shift_indices(ProgramCounterType pos, PointerDataType offset);
	void reset_index_shift();
	void delete_tokens(DeleteOp delete_op);
	void insert_tokens(InsertOp insert_op);
	void print_node(Node* node, int indent_level);
	void shift_pointers(std::vector<Token>& token_list, PointerDataType pos, PointerDataType offset);
	bool unary_func(std::function<Token(Token)> func);
	void binary_func(std::function<Token(Token, Token)> func);
	std::vector<Token> sys_call(int index, std::vector<Token> input);
	std::vector<Token> sys_print(std::vector<Token> input);

};