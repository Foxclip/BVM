#include "compiler.h"

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

std::vector<std::string> get_word_subtree(ProgramCounterType& index) {
	return std::vector<std::string>();
}

void expand_macro(Macro& macro) {

}

void replace_macros(std::vector<WordToken>& words) {
	std::set<Macro, decltype(&macro_cmp)> macros;
	enum DefState {
		DS_BEGIN,
		DS_NAME,
		DS_ARG,
		DS_ARGLIST,
		DS_BODY_BEGIN,
		DS_BODY,
	};
	DefState def_state = DS_BEGIN;
	Macro current_macro;
	ProgramCounterType parent_count = 0;
	for (ProgramCounterType i = 0; i < words.size(); i++) {
		WordToken current_word_token = words[i];
		try {
			if (def_state == DS_BEGIN) {
				if (current_word_token.str == "def") {
					def_state = DS_NAME;
				} else {
					auto it = macros.find(Macro(current_word_token.str));
					if (it != macros.end()) {
						expand_macro((Macro&)*it);
					} else {
						// ok
					}
				}
			} else if (def_state == DS_NAME) {
				if (utils::is_valid_word_prefix(current_word_token.str.front())) {
					current_macro = Macro();
					current_macro.name = current_word_token.str;
					def_state = DS_ARG;
				} else {
					throw std::runtime_error("Invalid macro name: " + current_word_token.str);
				}
			} else if (def_state == DS_ARG) {
				if (current_word_token.str == "list") {
					def_state = DS_ARGLIST;
				} else if (utils::is_valid_word_prefix(current_word_token.str.front())) {
					current_macro.arg_names.push_back(current_word_token.str);
					def_state = DS_BODY_BEGIN;
				} else {
					throw std::runtime_error("Invalid macro argument name: " + current_word_token.str);
				}
			} else if (def_state == DS_ARGLIST) {
				if (current_word_token.str == "end") {
					def_state = DS_BODY_BEGIN;
				} else if (utils::is_valid_word_prefix(current_word_token.str.front())) {
					current_macro.arg_names.push_back(current_word_token.str);
				} else {
					throw std::runtime_error("Invalid macro argument name: " + current_word_token.str);
				}
			} else if (def_state == DS_BODY_BEGIN) {
				if (utils::is_container_name(current_word_token.str)) {
					parent_count++;
					current_macro.body.push_back(current_word_token.str);
					def_state = DS_BODY;
				} else if (current_word_token.str == "end") {
					throw std::runtime_error("Macro body cannot begin with end token");
				} else {
					auto it = macros.find(Macro(current_word_token.str));
					if (it != macros.end()) {
						expand_macro((Macro&)*it);
					} else {
						current_macro.body.push_back(current_word_token.str);
						macros.insert(current_macro);
						def_state = DS_BEGIN;
					}
				}
			} else if (def_state == DS_BODY) {
				if (utils::is_container_name(current_word_token.str)) {
					parent_count++;
					current_macro.body.push_back(current_word_token.str);
				} else if (current_word_token.str == "end") {
					parent_count--;
					if (parent_count == 0) {
						current_macro.body.push_back(current_word_token.str);
						macros.insert(current_macro);
						def_state = DS_BEGIN;
					}
				} else {
					auto it = macros.find(Macro(current_word_token.str));
					if (it != macros.end()) {
						expand_macro((Macro&)*it);
					} else {
						current_macro.body.push_back(current_word_token.str);
					}
				}
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
		//replace_macros(words);
		replace_string_literals(words);
		replace_type_literals(words);
		LabelSet labels = create_labels(words);
		std::vector<Token> tokens = create_tokens(words, labels);
		return tokens;
	} catch (std::exception exc) {
		throw std::runtime_error(__FUNCTION__": " + std::string(exc.what()));
	}
}
