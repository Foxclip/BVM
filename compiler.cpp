#include "compiler.h"
#include <stack>

bool label_cmp(const Label& left, const Label& right) {
	return left.str < right.str;
}

bool macro_cmp(const Macro& left, const Macro& right) {
	return left.name < right.name;
}

std::vector<WordToken> tokenize(std::string str) {
	std::vector<WordToken> words;
	if (str.size() < 1) {
		return words;
	}
	enum SplitterState {
		STATE_SPACE,
		STATE_WORD,
		STATE_STRING,
		STATE_ESCAPE,
		STATE_COMMENT,
	};
	str += EOF;
	SplitterState state = STATE_SPACE;
	std::string current_word = "";
	ProgramCounterType current_line = 1;
	auto throw_unexp_char = [](char c, std::string w) {
		throw std::runtime_error("Current word: " + w + ", unexpected char: " + utils::char_to_str(c));
	};
	try {
		for (ProgramCounterType i = 0; i < str.size(); i++) {
			char current_char = str[i];
			if (current_char < -1) {
				throw std::runtime_error("Invalid char: " + std::to_string(current_char));
			}
			if (state == STATE_WORD) {
				if (isspace(current_char)) {
					words.push_back(WordToken(current_word, current_line));
					current_word = "";
					state = STATE_SPACE;
				} else if (current_char == '#') {
					words.push_back(WordToken(current_word, current_line));
					current_word = "";
					state = STATE_COMMENT;
				} else if (current_char == EOF) {
					words.push_back(WordToken(current_word, current_line));
					break;
				} else {
					current_word += current_char;
				}
			} else if (state == STATE_SPACE) {
				if (isspace(current_char)) {
					// ok
				} else if (current_char == '"') {
					current_word = "";
					state = STATE_STRING;
				} else if (current_char == '#') {
					state = STATE_COMMENT;
				} else if (current_char == EOF) {
					break;
				} else {
					current_word = "";
					current_word += current_char;
					state = STATE_WORD;
				}
			} else if (state == STATE_STRING) {
				if (current_char == '"') {
					std::string repl_esc = utils::replace_escape_seq(current_word);
					repl_esc.insert(repl_esc.begin(), '"');
					repl_esc.insert(repl_esc.end(), '"');
					words.push_back(WordToken(repl_esc, current_line));
					state = STATE_SPACE;
				} else if (current_char == '\\') {
					state = STATE_ESCAPE;
				} else if (current_char == EOF) {
					throw_unexp_char(current_char, current_word);
				} else {
					current_word += current_char;
				}
			} else if (state == STATE_ESCAPE) {
				if (current_char != '"') {
					current_word += '\\';
				}
				current_word += current_char;
				state = STATE_STRING;
			} else if (state == STATE_COMMENT) {
				if (utils::is_newline(current_char)) {
					state = STATE_SPACE;
				} else if (current_char == EOF) {
					break;
				}
			}
			if (utils::is_newline(current_char)) {
				current_line++;
			}
		}
	} catch (std::exception exc) {
		throw std::runtime_error("Line " + std::to_string(current_line) + ": " + std::string(exc.what()));
	}
	return words;
}

std::vector<WordToken> get_subtree(std::vector<WordToken>& words, ProgramCounterType index, MacroSet& macros, ProgramCounterType& last_index) {
	std::vector<WordToken> subtree;
	std::stack<TreeToken> parent_stack;
	ProgramCounterType i;
	for (i = index; i < words.size(); i++) {
		TreeToken current_token(words[i].str, i);
		ProgramCounterType arg_count = 0;
		InstructionInfo instr = get_instruction_info(current_token.str);
		if (instr.index >= 0) {
			arg_count = instr.arg_count;
			current_token.arg_count = arg_count;
			if (arg_count > 0) {
				parent_stack.push(current_token);
			}
			subtree.push_back(words[i]);
		} else {
			auto macro_it = macros.find(Macro(current_token.str));
			if (macro_it != macros.end()) {
				expand_macro(words, i, macros, (Macro&)*macro_it);
				i--;
				continue;
			} else {
				subtree.push_back(words[i]);
			}
		}
		ProgramCounterType current_index = i;
		ProgramCounterType current_last_index;
		bool first = true;
		while (!parent_stack.empty()) {
			TreeToken& current_parent = parent_stack.top();
			ProgramCounterType arg_offset = current_index - current_parent.first_index;
			bool arg_offset_end = arg_offset >= current_parent.arg_count;
			bool end_end = current_token.str == "end";
			auto exit_level = [&]() {
				if (first) {
					current_last_index = current_index;
					first = false;
				}
				current_index = parent_stack.top().first_index;
				parent_stack.pop();
			};
			if (arg_offset_end || end_end) {
				exit_level();
			} else {
				break;
			}
		}
		if (parent_stack.empty()) {
			break;
		}
	}
	last_index = i;
	return subtree;
}

void expand_macro(std::vector<WordToken>& words, ProgramCounterType index, MacroSet& macros, Macro& macro) {
	words.erase(words.begin() + index);
	std::vector<std::vector<WordToken>> args;
	for (ProgramCounterType i = 0; i < macro.arg_names.size(); i++) {
		ProgramCounterType last_index;
		std::vector<WordToken> arg_subtree = get_subtree(words, index, macros, last_index);
		args.push_back(arg_subtree);
		words.erase(words.begin() + index, words.begin() + last_index + 1);
	}
	for (ProgramCounterType i = 0; i < macro.body.size(); i++) {
		auto it = std::find(macro.arg_names.begin(), macro.arg_names.end(), macro.body[i].str);
		ProgramCounterType arg_index;
		if (it != macro.arg_names.end()) {
			arg_index = it - macro.arg_names.begin();
			words.insert(words.begin() + index + i, args[arg_index].begin(), args[arg_index].end());
		} else {
			words.insert(words.begin() + index + i, macro.body[i]);
		}
	}
}

void replace_macros(std::vector<WordToken>& words) {
	std::set<Macro, decltype(&macro_cmp)> macros(macro_cmp);
	enum DefState {
		STATE_BEGIN,
		STATE_NAME,
		STATE_ARG,
		STATE_ARGLIST,
		STATE_BODY,
	};
	DefState state = STATE_BEGIN;
	Macro current_macro;
	PointerDataType first_index;
	for (PointerDataType i = 0; i < words.size(); i++) {
		WordToken current_word_token = words[i];
		try {
			if (state == STATE_BEGIN) {
				if (current_word_token.str == "def") {
					first_index = i;
					state = STATE_NAME;
				} else {
					auto it = macros.find(Macro(current_word_token.str));
					if (it != macros.end()) {
						expand_macro(words, i, macros, (Macro&)*it);
						i--;
					} else {
						// ok
					}
				}
			} else if (state == STATE_NAME) {
				if (utils::is_valid_word_prefix(current_word_token.str.front())) {
					current_macro = Macro();
					current_macro.name = current_word_token.str;
					state = STATE_ARG;
				} else {
					throw std::runtime_error("Invalid macro name: " + current_word_token.str);
				}
			} else if (state == STATE_ARG) {
				if (current_word_token.str == "list") {
					state = STATE_ARGLIST;
				} else if (utils::is_valid_word_prefix(current_word_token.str.front())) {
					current_macro.arg_names.push_back(current_word_token.str);
					state = STATE_BODY;
				} else {
					throw std::runtime_error("Invalid macro argument name: " + current_word_token.str);
				}
			} else if (state == STATE_ARGLIST) {
				if (current_word_token.str == "end") {
					state = STATE_BODY;
				} else if (utils::is_valid_word_prefix(current_word_token.str.front())) {
					current_macro.arg_names.push_back(current_word_token.str);
				} else {
					throw std::runtime_error("Invalid macro argument name: " + current_word_token.str);
				}
			} else if (state == STATE_BODY) {
				ProgramCounterType last_index;
				current_macro.body = get_subtree(words, i, macros, last_index);
				macros.insert(current_macro);
				words.erase(words.begin() + first_index, words.begin() + last_index + 1);
				i = first_index - 1;
				state = STATE_BEGIN;
			}
		} catch (std::exception exc) {
			throw std::runtime_error("Line " + std::to_string(current_word_token.line) + ": " + std::string(exc.what()));
		}
	}
}

void replace_string_literals(std::vector<WordToken>& words) {
	for (ProgramCounterType i = 0; i < words.size(); i++) {
		WordToken current_word_token = words[i];
		try {
			std::string current_word = current_word_token.str;
			if (current_word.size() > 1 && current_word.front() == '"' && current_word.back() == '"') {
				std::string string_content = current_word.substr(1, current_word.size() - 2);
				std::string list_display_string = "list #\"" + utils::string_conv(string_content) + "\"";
				WordToken list_token = WordToken("list", list_display_string, current_word_token.line);
				words[i] = list_token;
				for (ProgramCounterType char_i = 0; char_i < string_content.size(); char_i++) {
					char c = string_content[char_i];
					std::string char_string = std::to_string(c);
					std::string char_display_string = char_string + " #'" + utils::char_to_str(c) + "'";
					WordToken char_token = WordToken(char_string, char_display_string, current_word_token.line);
					ProgramCounterType char_token_index = i + char_i + 1;
					words.insert(words.begin() + char_token_index, char_token);
				}
				WordToken end_token = WordToken("end", current_word_token.line);
				ProgramCounterType end_token_index = i + string_content.size() + 1;
				words.insert(words.begin() + end_token_index, end_token);
			}
		} catch (std::exception exc) {
			throw std::runtime_error("Line " + std::to_string(current_word_token.line) + ": " + std::string(exc.what()));
		}
	}
}

void replace_type_literals(std::vector<WordToken>& words) {
	for (ProgramCounterType i = 0; i < words.size(); i++) {
		WordToken current_word_token = words[i];
		try {
			std::string current_word = current_word_token.str;
			token_type type = string_to_type(current_word);
			if (type != type_unknown) {
				int type_index = (int)type;
				words[i].str = std::to_string(type_index);
			}
		} catch (std::exception exc) {
			throw std::runtime_error("Line " + std::to_string(current_word_token.line) + ": " + std::string(exc.what()));
		}
	}
}

LabelSet create_labels(std::vector<WordToken>& words) {
	LabelSet labels(label_cmp);
	for (ProgramCounterType i = 0; i < words.size(); i++) {
		WordToken current_word_token = words[i];
		try {
			std::string current_word = current_word_token.str;
			if (current_word.front() == ':') {
				std::string label_str = current_word.substr(1, current_word.size() - 1);
				Label new_label(label_str, (PointerDataType)i - 1);
				if (labels.find(new_label) != labels.end()) {
					throw std::runtime_error("Duplicate label: " + label_str);
				}
				token_type type = string_to_type(label_str);
				if (type != type_unknown) {
					throw std::runtime_error("Label cannot be a type name: " + label_str);
				}
				InstructionInfo instr = get_instruction_info(label_str);
				if (instr.index >= 0) {
					throw std::runtime_error("Label cannot be an instruction name: " + label_str);
				}
				labels.insert(new_label);
				words.erase(words.begin() + i);
				i--;
			}
		} catch (std::exception exc) {
			throw std::runtime_error("Line " + std::to_string(current_word_token.line) + ": " + std::string(exc.what()));
		}
	}
	return labels;
}

std::vector<Token> create_tokens(std::vector<WordToken>& words, LabelSet labels) {
	std::vector<Token> tokens;
	for (ProgramCounterType i = 0; i < words.size(); i++) {
		WordToken current_word_token = words[i];
		try {
			std::string str = current_word_token.str;
			Token new_token;
			auto it = labels.find(Label(str, 0));
			if (it != labels.end()) {
				PointerDataType relative_address = (*it).token_index - i;
				new_token = Token(str, Token::get_token_type<PointerTokenType>());
				new_token.set_data<PointerDataType>(relative_address);
				new_token.str = new_token.to_string();
			} else {
				new_token = Token(str);
			}
			new_token.orig_str = current_word_token.display_str;
			tokens.push_back(new_token);
		} catch (std::exception exc) {
			throw std::runtime_error("Line " + std::to_string(current_word_token.line) + ": " + std::string(exc.what()));
		}
	}
	return tokens;
}

std::vector<Token> compile(std::string str) {
	try {
		std::vector<WordToken> words = tokenize(str);
		replace_macros(words);
		replace_string_literals(words);
		replace_type_literals(words);
		LabelSet labels = create_labels(words);
		std::vector<Token> tokens = create_tokens(words, labels);
		return tokens;
	} catch (std::exception exc) {
		throw std::runtime_error(__FUNCTION__": " + std::string(exc.what()));
	}
}

TreeToken::TreeToken() {}

TreeToken::TreeToken(std::string str, ProgramCounterType first_index) {
	this->str = str;
	this->first_index = first_index;
}
