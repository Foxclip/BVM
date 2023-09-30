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

void throwUnexpectedCharException(char c, std::string current_word, ProgramCounterType line);

struct Label {
	std::string str;
	PointerDataType token_index;

	Label(std::string str, PointerDataType token_index);
};

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
	bool print_iterations = false;

	Program(std::string str);
	void print_tokens(bool print_program_counter = true);
	void print_nodes();
	std::vector<Token> execute();

private:
	PointerDataType program_counter = 0;

	std::vector<Token> tokenize(std::string str);
	void parse();
	PointerDataType token_index(std::vector<Token>& token_list, PointerDataType index);
	Token& get_token(std::vector<Token>& token_list, PointerDataType index);
	Token& rel_token(std::vector<Token>& token_list, PointerDataType offset);
	std::unique_ptr<Node> parse_token(std::vector<Token>& token_list, PointerDataType token_index, Node* parent_node, PointerDataType& new_token_index);
	void print_node(Node* node, int indent_level);
	void shift_pointers(std::vector<Token>& token_list, PointerDataType pos, PointerDataType offset);
	bool binary_func(std::function<Token(Token, Token)> func);

};