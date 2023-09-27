#pragma once

#include <vector>
#include <functional>
#include <string>
#include <stack>
#include <iostream>
#include <algorithm>
#include "utils.h"

const long MAX_ITERATIONS = 1000;
typedef int DefaultNumType;
struct PointerTokenType {
	using data_type = long;
};
typedef PointerTokenType::data_type PointerDataType;
struct InstructionTokenType {
	using data_type = int;
};
typedef InstructionTokenType::data_type InstructionDataType;

struct InstructionDef {
	std::string str;
	int arg_count;
	InstructionDef(std::string str, int arg_count) {
		this->str = str;
		this->arg_count = arg_count;
	}
};

struct InstructionInfo {
	std::string str;
	int arg_count;
	int index;
	InstructionInfo(std::string str, int arg_count, int index) {
		this->str = str;
		this->arg_count = arg_count;
		this->index = index;
	}
};

const std::vector<InstructionDef> INSTRUCTION_LIST = 
{
	InstructionDef("add", 2),
	InstructionDef("sub", 2),
	InstructionDef("mul", 2),
	InstructionDef("div", 2),
	InstructionDef("mod", 2),
	InstructionDef("pow", 2),
	InstructionDef("cmp", 2),
	InstructionDef("lt", 2),
	InstructionDef("gt", 2),
	InstructionDef("cpy", 2),
	InstructionDef("del", 1),
	InstructionDef("repl", 2),
	InstructionDef("replp", 2),
	InstructionDef("get", 1),
	InstructionDef("ins", 2),
	InstructionDef("if", 3),
	InstructionDef("list", -1),
	InstructionDef("end", 0),
	InstructionDef("p", 1),
};

void throwUnexpectedCharException(char c, std::string current_word);

enum token_type {
	type_int,
	type_long,
	type_uint,
	type_ulong,
	type_float,
	type_double,
	type_instr,
	type_ptr,
};

template <typename T>
token_type get_token_type() {
	if constexpr (std::is_same_v<T, int>) {
		return type_int;
	} else if constexpr (std::is_same_v<T, long>) {
		return type_long;
	} else if constexpr (std::is_same_v<T, unsigned int>) {
		return type_uint;
	} else if constexpr (std::is_same_v<T, unsigned long>) {
		return type_ulong;
	} else if constexpr (std::is_same_v<T, float>) {
		return type_float;
	} else if constexpr (std::is_same_v<T, double>) {
		return type_double;
	} else if constexpr (std::is_same_v<T, InstructionTokenType>) {
		return type_instr;
	} else if constexpr (std::is_same_v<T, PointerTokenType>) {
		return type_ptr;
	} else {
		static_assert(true, "Unknown token_data type");
	}
}

struct Token {
	// adding new data type:
	// token_type enum
	// get_token_type func
	// get_data func
	// get data cast func
	// set_data func
	// is_num func (?)
	// is_num_or_ptr func (?)
	// to_string func
	// operator==
	// CALL_BINARY_FUNC macro
	// get_return_type func

	union token_data {
		int m_int;
		long m_long;
		unsigned int m_uint;
		unsigned long m_ulong;
		float m_float;
		double m_double;
		InstructionDataType m_instr;
		PointerDataType m_ptr;
	};
	std::string str;
	token_type type;
	token_data data;

	Token();
	Token(std::string str, token_type type);
	bool is_num();
	bool is_ptr();
	bool is_num_or_ptr();
	std::string to_string() const;
	friend bool operator==(const Token& first, const Token& second);

	template <typename T>
	T get_data() const {
		if constexpr (std::is_same_v<T, int>) {
			return data.m_int;
		} else if constexpr (std::is_same_v<T, long>) {
			return data.m_long;
		} else if constexpr (std::is_same_v<T, unsigned int>) {
			return data.m_uint;
		} else if constexpr (std::is_same_v<T, unsigned long>) {
			return data.m_ulong;
		} else if constexpr (std::is_same_v<T, float>) {
			return data.m_float;
		} else if constexpr (std::is_same_v<T, double>) {
			return data.m_double;
		} else {
			bool fail = T::unknown_type;
			static_assert(true, "Unknown token_data type");
		}
	}

	template <typename T>
	T get_data_cast() const {
		switch (type) {
			case type_int:
				return (T)data.m_int;
			case type_uint:
				return (T)data.m_uint;
			case type_long:
				return (T)data.m_long;
			case type_ulong:
				return (T)data.m_ulong;
			case type_float:
				return (T)data.m_float;
			case type_double:
				return (T)data.m_double;
			default:
				throw std::runtime_error("Unknown token_data type: " + std::to_string(type));
		}
	}

	template <typename T>
	void set_data(T new_data) {
		if constexpr (std::is_same_v<T, int>) {
			data.m_int = new_data;
		} else if constexpr (std::is_same_v<T, long>) {
			data.m_long = new_data;
		} else if constexpr (std::is_same_v<T, unsigned int>) {
			data.m_uint = new_data;
		} else if constexpr (std::is_same_v<T, unsigned long>) {
			data.m_ulong = new_data;
		} else if constexpr (std::is_same_v<T, float>) {
			data.m_float = new_data;
		} else if constexpr (std::is_same_v<T, double>) {
			data.m_double = new_data;
		} else {
			static_assert(true, "Unknown token_data type");
		}
	}
};

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
	void print_tokens();
	void print_nodes();
	std::vector<long> execute();

private:
	PointerDataType program_counter = 0;
	std::vector<long> inputs = { 5, 6, 7 };

	std::vector<Token> tokenize(std::string str);
	void parse();
	PointerDataType token_index(std::vector<Token>& token_list, PointerDataType index);
	Token& get_token(std::vector<Token>& token_list, PointerDataType index);
	Token& rel_token(std::vector<Token>& token_list, PointerDataType offset);
	std::unique_ptr<Node> parse_token(std::vector<Token>& token_list, PointerDataType token_index, Node* parent_node, PointerDataType& new_token_index);
	void print_node(Node* node, int indent_level);
	void shift_pointers(std::vector<Token>& token_list, PointerDataType pos, PointerDataType offset);
	InstructionInfo get_instruction_info(std::string token);
	token_type get_return_type(token_type type1, token_type type2);

	template<typename TRet>
	bool binary_func(std::function<TRet(TRet, TRet)> func) {
		if (rel_token(tokens, 1).is_num_or_ptr() && rel_token(tokens, 2).is_num_or_ptr()) {
			shift_pointers(tokens, program_counter, -2);
			TRet val1 = rel_token(tokens, 1).get_data_cast<TRet>();
			TRet val2 = rel_token(tokens, 2).get_data_cast<TRet>();
			TRet result = func(val1, val2);
			tokens.erase(tokens.begin() + program_counter);
			tokens.erase(tokens.begin() + program_counter);
			rel_token(tokens, 0).str = std::to_string(result);
			rel_token(tokens, 0).set_data<TRet>(result);
			rel_token(tokens, 0).type = get_token_type<TRet>();
			return true;
		}
		return false;
	}

};