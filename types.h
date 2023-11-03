#pragma once
#include <string>

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
	using data_type = Int32Type;
};
typedef InstructionTokenType::data_type InstructionDataType;

// adding new token type:
// string_to_type
// type_to_string
// token_type enum
// utils::is_number
// is_num_or_ptr func (?)
// Token::Token
// Token::get_token_type
// Token::get_data
// Token::get data cast
// Token::set_data
// Token::to_string
// Token::cast
// Token::div
// Token::mod
// Token::operator==
// Token::numeric_compare
// Token::approx_compare
// TOKEN_BINARY_OP macro
// Token::get_return_type
// Token::is_int_type
// parsing literals in tokenize func
// is_int_suffix func and is_float_suffix func in utils

enum token_type {
	type_int32,
	type_int64,
	type_uint32,
	type_uint64,
	type_float,
	type_double,
	type_instr,
	type_ptr,
	type_unknown, // keep last
};

const token_type INT_ZERO_DIV_RESULT_TYPE = type_float;
const token_type CMP_RETURN_TYPE = type_int32;

token_type string_to_type(std::string str);
std::string type_to_string(token_type type);