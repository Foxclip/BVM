#pragma once

#include "token.h"

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
bool label_cmp(const Label& left, const Label& right);
typedef std::set<Label, decltype(&label_cmp)> LabelSet;

struct Macro {
	std::string name;
	std::vector<std::string> arg_names;
	std::vector<std::string> body;
	Macro() {}
	Macro(std::string name) {
		this->name = name;
	}
};
bool macro_cmp(const Macro& left, const Macro& right);

std::vector<WordToken> tokenize(std::string str);
std::vector<std::string> get_word_subtree(ProgramCounterType& index);
void expand_macro(Macro& macro);
void replace_macros(std::vector<WordToken>& words);
void replace_string_literals(std::vector<WordToken>& words);
void replace_type_literals(std::vector<WordToken>& words);
LabelSet create_labels(std::vector<WordToken>& words);
std::vector<Token> create_tokens(std::vector<WordToken>& words, LabelSet labels);
std::vector<Token> compile(std::string str);