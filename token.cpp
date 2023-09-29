#include "token.h"

Token::Token() {}

Token::Token(std::string str, token_type type) {
	this->str = str;
	this->type = type;
}

Token::Token(std::string str) {
	this->str = str;
	if (utils::is_number_prefix(str.front())) {
		if (isdigit(str.back())) {
			type = type_int32;
		} else {
			switch (str.back()) {
				case 'L': type = type_int64; break;
				case 'u': type = type_uint32; break;
				case 'U': type = type_uint64; break;
				case 'f': type = type_float; break;
				case 'd': type = type_double; break;
				case 'p': type = type_ptr; break;
				default: throw std::runtime_error("Unknown number suffix: " + std::string(1, str.back()));
			}
		}
		switch (type) {
			case type_int32: set_data<Int32Type>(std::stoi(str)); break;
			case type_int64: set_data<Int64Type>(std::stoll(str)); break;
			case type_uint32: set_data<Uint32Type>(std::stoul(str)); break;
			case type_uint64: set_data<Uint64Type>(std::stoull(str)); break;
			case type_float: set_data<FloatDataType>(std::stof(str)); break;
			case type_double: set_data<DoubleDataType>(std::stod(str)); break;
			case type_ptr: set_data<PointerDataType>(POINTER_DATA_PARSE_FUNC(str)); break;
			default: throw std::runtime_error("Unknown token_data type: " + std::to_string(type));
		}
	} else {
		type = get_token_type<InstructionTokenType>();
		set_data<InstructionDataType>(get_instruction_info(str).index);
	}
}

bool Token::is_num() {
	switch (type) {
		case type_int32:
		case type_int64:
		case type_uint32:
		case type_uint64:
		case type_float:
		case type_double:
			return true;
		default:
			return false;
	}
}

bool Token::is_ptr() {
	return type == type_ptr;
}

bool Token::is_num_or_ptr() {
	return is_num() || is_ptr();
}

std::string Token::to_string() const {
	switch (type) {
		case type_int32:
			return std::to_string(data.m_int32);
		case type_int64:
			return std::to_string(data.m_int64) + "L";
		case type_uint32:
			return std::to_string(data.m_uint32) + "u";
		case type_uint64:
			return std::to_string(data.m_uint64) + "U";
		case type_float:
			return std::to_string(data.m_float) + "f";
		case type_double:
			return std::to_string(data.m_double) + "d";
		case type_instr:
			return INSTRUCTION_LIST[get_data<InstructionDataType>()].str;
		case type_ptr:
			return std::to_string(get_data<PointerDataType>()) + "p";
		default:
			throw std::runtime_error("Unknown token_data type: " + std::to_string(type));
	}
}

token_type Token::get_return_type(token_type type1, token_type type2) {
	if (type1 == type_double || type2 == type_double) {
		return type_double;
	} else if (type1 == type_float || type2 == type_float) {
		return type_float;
	} else if (type1 == type_ptr || type2 == type_ptr) {
		return type_ptr;
	} else if (type1 == type_uint64 || type2 == type_uint64) {
		return type_uint64;
	} else if (type1 == type_int64 || type2 == type_int64) {
		return type_int64;
	} else if (type1 == type_uint32 || type2 == type_uint32) {
		return type_uint32;
	} else if (type1 == type_int32 || type2 == type_int32) {
		return type_int32;
	} else {
		throw std::runtime_error("Cannot determine return type: " + std::to_string(type1) + " " + std::to_string(type2));
	}
}

std::string Token::tokens_to_str(std::vector<Token> tokens) {
	std::string str;
	for (ProgramCounterType i = 0; i < tokens.size(); i++) {
		str += tokens[i].to_string();
		if (i != tokens.size() - 1) {
			str += " ";
		}
	}
	return str;
}

std::vector<Token> Token::str_to_tokens(std::string str) {
	std::vector<Token> tokens;
	std::vector<std::string> strings = utils::split_string(str, ' ');
	for (ProgramCounterType i = 0; i < strings.size(); i++) {
		std::string str = strings[i];
		Token token(str);
		tokens.push_back(token);
	}
	return tokens;
}

#define TOKEN_BINARY_OP_CASE(TP1, TP2, FUNC) \
	case TP1: \
		{ \
			auto func = [](TP2 a, TP2 b) { FUNC }; \
			TP2 a_data = first.get_data_cast<TP2>(); \
			TP2 b_data = second.get_data_cast<TP2>(); \
			TP2 result_data = func(a_data, b_data); \
			result.set_data<TP2>(result_data); \
		} \
		break;

#define TOKEN_BINARY_OP(FUNC) \
	token_type return_type = get_return_type(first.type, second.type); \
	Token result; \
	result.type = return_type; \
	switch (return_type) { \
		TOKEN_BINARY_OP_CASE(type_int32, Int32Type, FUNC) \
		TOKEN_BINARY_OP_CASE(type_int64, Int64Type, FUNC) \
		TOKEN_BINARY_OP_CASE(type_uint32, Uint32Type, FUNC) \
		TOKEN_BINARY_OP_CASE(type_uint64, Uint64Type, FUNC) \
		TOKEN_BINARY_OP_CASE(type_float, FloatDataType, FUNC) \
		TOKEN_BINARY_OP_CASE(type_double, DoubleDataType, FUNC) \
		TOKEN_BINARY_OP_CASE(type_instr, InstructionDataType, FUNC) \
		TOKEN_BINARY_OP_CASE(type_ptr, PointerDataType, FUNC) \
		default: \
			throw std::runtime_error("Unknown token_data type: " + std::to_string(first.type)); \
	} \
	result.str = result.to_string(); \
	return result; \

Token Token::add(const Token& first, const Token& second) {
	TOKEN_BINARY_OP( return a + b; )
}

Token Token::sub(const Token& first, const Token& second) {
	TOKEN_BINARY_OP( return a - b; )
}

Token Token::mul(const Token& first, const Token& second) {
	TOKEN_BINARY_OP( return a * b; )
}

Token Token::div(const Token& first, const Token& second) {
	TOKEN_BINARY_OP( return a / b; )
}

#define FUNC return utils::mod(a, b);
Token Token::mod(const Token& first, const Token& second) {
	token_type return_type = get_return_type(first.type, second.type);
	if (return_type == type_float || return_type == type_double) {
		return_type = type_int64;
	}
	Token result;
	result.type = return_type;
	switch (return_type) {
		TOKEN_BINARY_OP_CASE(type_int32, Int32Type, FUNC)
			TOKEN_BINARY_OP_CASE(type_int64, Int64Type, FUNC)
			TOKEN_BINARY_OP_CASE(type_uint32, Uint32Type, FUNC)
			TOKEN_BINARY_OP_CASE(type_uint64, Uint64Type, FUNC)
			TOKEN_BINARY_OP_CASE(type_instr, InstructionDataType, FUNC)
			TOKEN_BINARY_OP_CASE(type_ptr, PointerDataType, FUNC)
		default:
			throw std::runtime_error("Unknown token_data type: " + std::to_string(first.type));
	}
	result.str = result.to_string();
	return result;
}

Token Token::pow(const Token& first, const Token& second) {
	TOKEN_BINARY_OP( return std::pow(a, b); )
}

Token Token::cmp(const Token& first, const Token& second) {
	TOKEN_BINARY_OP( return a == b; )
}

Token Token::lt(const Token& first, const Token& second) {
	TOKEN_BINARY_OP( return a < b; )
}

Token Token::gt(const Token& first, const Token& second) {
	TOKEN_BINARY_OP( return a > b; )
}

bool operator==(const Token& first, const Token& second) {
	if (first.type == second.type) {
		switch (first.type) {
			case type_int32:
				return first.data.m_int32 == second.data.m_int32;
			case type_int64:
				return first.data.m_int64 == second.data.m_int64;
			case type_uint32:
				return first.data.m_uint32 == second.data.m_uint32;
			case type_uint64:
				return first.data.m_uint64 == second.data.m_uint64;
			case type_float:
				return first.data.m_float == second.data.m_float;
			case type_double:
				return first.data.m_double == second.data.m_double;
			case type_instr:
				return first.get_data<InstructionDataType>() == second.get_data<InstructionDataType>();
			case type_ptr:
				return first.get_data<PointerDataType>() == second.get_data<PointerDataType>();
			default:
				throw std::runtime_error("Unknown token_data type: " + std::to_string(first.type));
		}
	}
	return false;
}