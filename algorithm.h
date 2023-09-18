#pragma once

#include <vector>
#include <functional>
#include <string>
#include <stack>
#include <iostream>
#include <algorithm>
#include "utils.h"

const long MAX_ITERATIONS = 20;
typedef std::pair<std::string, int> InstructionDef;
const std::vector<InstructionDef> INSTRUCTION_LIST = {
	std::pair("Val", 0),
	std::pair("Inp", 1),
	std::pair("Add", 2),
	std::pair("Mul", 2),
	std::pair("Cpy", 2),
	std::pair("Node", 2),
	std::pair("Del", 1),
	std::pair("Cmp", 2),
	std::pair("If", 3),
};

void throwUnexpectedCharException(char c, std::string current_word);

struct Token {
	long index = 0;
	std::string str;
	long num_value = 0;

	Token();
	Token(long index, std::string str, long num_value);
	std::string to_string();
	std::string _to_string();
	friend bool operator==(const Token& first, const Token& second);
};

struct Label {
	std::string str;
	long token_index;

	Label(std::string str, long token_index);
};

std::vector<Token> tokenize(std::string str);

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

	Program(std::string str);
	void print_tokens();
	void print_nodes();
	std::vector<long> execute();
	void parse();

private:
	unsigned long program_counter = 0;
	std::vector<long> inputs = { 5, 6, 7 };

	long token_index(std::vector<Token>& token_list, long index);
	Token& get_token(std::vector<Token>& token_list, long index);
	Token& rel_token(std::vector<Token>& token_list, long offset);
	std::unique_ptr<Node> parse_token(std::vector<Token>& token_list, long token_index, Node* parent_node, long& new_token_index);
	void print_node(Node* node, int indent_level);
};