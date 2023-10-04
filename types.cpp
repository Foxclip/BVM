#include "types.h"

token_type string_to_type(std::string str) {
	if (str == "int32") {
		return type_int32;
	} else if (str == "int64") {
		return type_int64;
	} else if (str == "uint32") {
		return type_uint32;
	} else if (str == "uint64") {
		return type_uint64;
	} else if (str == "float") {
		return type_float;
	} else if (str == "double") {
		return type_double;
	} else if (str == "instr") {
		return type_instr;
	} else if (str == "ptr") {
		return type_ptr;
	} else {
		return type_unknown;
	}
}

std::string type_to_string(token_type type) {
	switch (type) {
		case type_int32: return "int32"; break;
		case type_int64: return "int64"; break;
		case type_uint32: return "uint32"; break;
		case type_uint64: return "uint64"; break;
		case type_float: return "float"; break;
		case type_double: return "double"; break;
		case type_instr: return "instr"; break;
		case type_ptr: return "ptr"; break;
		default: return "unknown";
	}
}