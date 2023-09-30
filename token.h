#pragma once

#include <string>
#include "utils.h"
#include "instruction.h"

typedef unsigned long long ProgramCounterType;
typedef long long FloatModConversionType;
typedef int Int32Type;
typedef unsigned int Uint32Type;
typedef long long Int64Type;
typedef unsigned long long Uint64Type;
typedef float FloatDataType;
typedef double DoubleDataType;
typedef int BoolType;

struct PointerTokenType {
	using data_type = Int64Type;
};
#define POINTER_DATA_PARSE_FUNC std::stoull
typedef PointerTokenType::data_type PointerDataType;

struct InstructionTokenType {
	using data_type = Uint32Type;
};
typedef InstructionTokenType::data_type InstructionDataType;

enum token_type {
	type_int32,
	type_int64,
	type_uint32,
	type_uint64,
	type_float,
	type_double,
	type_instr,
	type_ptr,
};

class Token {
public:
	// adding new data type:
	// token_type enum
	// get_token_type func
	// get_data func
	// get data cast func
	// set_data func
	// is_num func (?)
	// is_num_or_ptr func (?)
	// Token::to_string
	// Token::operator==
	// Token::approx_compare
	// TOKEN_BINARY_OP macro
	// get_return_type func
	// parsing literals in tokenize func
	// is_int_suffix func and is_float_suffix func in utils

	std::string str;
	token_type type = type_int32;

	Token();
	Token(std::string str, token_type type);
	Token(std::string str);
	bool is_num();
	bool is_ptr();
	bool is_num_or_ptr();
	std::string to_string() const;
	static token_type get_return_type(token_type type1, token_type type2);
	static std::string tokens_to_str(std::vector<Token> tokens);
	static std::vector<Token> str_to_tokens(std::string str);
	friend bool operator==(const Token& first, const Token& second);
	friend bool approx_compare(const Token& first, const Token& second, float epsilon);
	static Token add(const Token& first, const Token& second);
	static Token sub(const Token& first, const Token& second);
	static Token mul(const Token& first, const Token& second);
	static Token div(const Token& first, const Token& second);
	static Token mod(const Token& first, const Token& second);
	static Token pow(const Token& first, const Token& second);
	static Token cmp(const Token& first, const Token& second);
	static Token lt(const Token& first, const Token& second);
	static Token gt(const Token& first, const Token& second);

	template <typename T>
	static token_type get_token_type() {
		if constexpr (std::is_same_v<T, Int32Type>) {
			return type_int32;
		} else if constexpr (std::is_same_v<T, Int64Type>) {
			return type_int64;
		} else if constexpr (std::is_same_v<T, Uint32Type>) {
			return type_uint32;
		} else if constexpr (std::is_same_v<T, Uint64Type>) {
			return type_uint64;
		} else if constexpr (std::is_same_v<T, FloatDataType>) {
			return type_float;
		} else if constexpr (std::is_same_v<T, DoubleDataType>) {
			return type_double;
		} else if constexpr (std::is_same_v<T, InstructionTokenType>) {
			return type_instr;
		} else if constexpr (std::is_same_v<T, PointerTokenType>) {
			return type_ptr;
		} else {
			static_assert(true, "Unknown token_data type");
		}
	}

	template <typename T>
	T get_data() const {
		if constexpr (std::is_same_v<T, Int32Type>) {
			return data.m_int32;
		} else if constexpr (std::is_same_v<T, Int64Type>) {
			return data.m_int64;
		} else if constexpr (std::is_same_v<T, Uint32Type>) {
			return data.m_uint32;
		} else if constexpr (std::is_same_v<T, Uint64Type>) {
			return data.m_uint64;
		} else if constexpr (std::is_same_v<T, FloatDataType>) {
			return data.m_float;
		} else if constexpr (std::is_same_v<T, DoubleDataType>) {
			return data.m_double;
		} else {
			bool fail = T::unknown_type;
			static_assert(true, "Unknown token_data type");
		}
	}

	template <typename T>
	T get_data_cast() const {
		switch (type) {
			case type_int32: return (T)data.m_int32;
			case type_uint32: return (T)data.m_uint32;
			case type_int64: return (T)data.m_int64;
			case type_uint64: return (T)data.m_uint64;
			case type_float: return (T)data.m_float;
			case type_double: return (T)data.m_double;
			case type_ptr: return (T)get_data<PointerDataType>();
			case type_instr: return (T)get_data<InstructionDataType>();
			default: throw std::runtime_error("Unknown token_data type: " + std::to_string(type));
		}
	}

	template <typename T>
	void set_data(T new_data) {
		if constexpr (std::is_same_v<T, Int32Type>) {
			data.m_int32 = new_data;
		} else if constexpr (std::is_same_v<T, Int64Type>) {
			data.m_int64 = new_data;
		} else if constexpr (std::is_same_v<T, Uint32Type>) {
			data.m_uint32 = new_data;
		} else if constexpr (std::is_same_v<T, Uint64Type>) {
			data.m_uint64 = new_data;
		} else if constexpr (std::is_same_v<T, FloatDataType>) {
			data.m_float = new_data;
		} else if constexpr (std::is_same_v<T, DoubleDataType>) {
			data.m_double = new_data;
		} else {
			static_assert(true, "Unknown token_data type");
		}
	}

private:
	union token_data {
		Int32Type m_int32;
		Int64Type m_int64;
		Uint32Type m_uint32;
		Uint64Type m_uint64;
		FloatDataType m_float;
		DoubleDataType m_double;
		InstructionDataType m_instr;
		PointerDataType m_ptr;
	};
	token_data data = { 0 };
};