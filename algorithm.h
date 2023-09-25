#pragma once

#include <vector>
#include <functional>
#include <string>
#include <stack>
#include <iostream>
#include <algorithm>
#include "utils.h"

const long MAX_ITERATIONS = 1000;
typedef std::pair<std::string, int> InstructionDef;
const std::vector<InstructionDef> INSTRUCTION_LIST = {
	std::pair("val", 0),
	std::pair("inp", 1),
	std::pair("add", 2),
	std::pair("sub", 2),
	std::pair("mul", 2),
	std::pair("div", 2),
	std::pair("mod", 2),
	std::pair("pow", 2),
	std::pair("cmp", 2),
	std::pair("lt", 2),
	std::pair("gt", 2),
	std::pair("cpy", 2),
	std::pair("del", 1),
	std::pair("repl", 2),
	std::pair("if", 3),
	std::pair("list", -1),
	std::pair("end", 0),
	std::pair("p", 1),
};

void throwUnexpectedCharException(char c, std::string current_word);

struct Token {
	long index = 0;
	std::string str;
	long num_value = 0;
	bool pointer = false;

	Token();
	Token(long index, std::string str, long num_value);
	std::string to_string();
	friend bool operator==(const Token& first, const Token& second);
};

struct Label {
	std::string str;
	long token_index;

	Label(std::string str, long token_index);
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
	void print_tokens();
	void print_nodes();
	std::vector<long> execute();

private:
	unsigned long program_counter = 0;
	std::vector<long> inputs = { 5, 6, 7 };

	std::vector<Token> tokenize(std::string str);
	void parse();
	long token_index(std::vector<Token>& token_list, long index);
	Token& get_token(std::vector<Token>& token_list, long index);
	Token& rel_token(std::vector<Token>& token_list, long offset);
	std::unique_ptr<Node> parse_token(std::vector<Token>& token_list, long token_index, Node* parent_node, long& new_token_index);
	void print_node(Node* node, int indent_level);
	bool binary_func(std::function<long(long, long)> func);
	void shift_pointers(std::vector<Token>& token_list, long pos, long offset);
	InstructionDef get_instruction_info(std::string token);
};