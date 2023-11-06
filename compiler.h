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
	std::vector<WordToken> body;
	Macro() {}
	Macro(std::string name) {
		this->name = name;
	}
};
bool macro_cmp(const Macro& left, const Macro& right);
typedef std::set<Macro, decltype(&macro_cmp)> MacroSet;

struct TreeToken {
	std::string str;
	ProgramCounterType first_index;
	ProgramCounterType parent_index;
	ProgramCounterType arg_count;
	TreeToken();
	TreeToken(std::string str, ProgramCounterType first_index);
};

class Compiler {
public:
	std::vector<Token> compile(std::string str);

private:
	std::vector<WordToken> words;
	MacroSet macros = MacroSet(macro_cmp);
	LabelSet labels = LabelSet(label_cmp);
	std::vector<Token> tokens;

	std::vector<WordToken> get_subtree(ProgramCounterType index, ProgramCounterType& last_index);
	void expand_macro(ProgramCounterType index, Macro& macro);
	void tokenize(std::string str);
	void replace_macros();
	void replace_string_literals();
	void replace_type_literals();
	void create_labels();
	void create_tokens();

};