#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include "types.h"
#include <set>

struct InstructionDef {
	std::string str;
	ProgramCounterType arg_count;
	InstructionDef(std::string str, ProgramCounterType arg_count) {
		this->str = str;
		this->arg_count = arg_count;
	}
};

bool operator<(const InstructionDef& left, const InstructionDef& right);

struct InstructionInfo {
	std::string str = "";
	ProgramCounterType arg_count = 0;
	int index = -1;
	InstructionInfo() { }
	InstructionInfo(std::string str, ProgramCounterType arg_count, int index) {
		this->str = str;
		this->arg_count = arg_count;
		this->index = index;
	}
	InstructionInfo(InstructionDef def, int index) {
		this->str = def.str;
		this->arg_count = def.arg_count;
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
	InstructionDef("log", 1),
	InstructionDef("log2", 1),
	InstructionDef("sin", 1),
	InstructionDef("cos", 1),
	InstructionDef("tan", 1),
	InstructionDef("asin", 1),
	InstructionDef("acos", 1),
	InstructionDef("atan", 1),
	InstructionDef("atan2", 2),
	InstructionDef("floor", 1),
	InstructionDef("ceil", 1),
	InstructionDef("cmp", 2),
	InstructionDef("lt", 2),
	InstructionDef("gt", 2),
	InstructionDef("and", 2),
	InstructionDef("or", 2),
	InstructionDef("xor", 2),
	InstructionDef("not", 1),
	InstructionDef("cpy", 2),
	InstructionDef("del", 1),
	InstructionDef("set", 2),
	InstructionDef("repl", 2),
	InstructionDef("get", 1),
	InstructionDef("ins", 2),
	InstructionDef("move", 2),
	InstructionDef("mrep", 2),
	InstructionDef("if", 3),
	InstructionDef("list", (ProgramCounterType)-1),
	InstructionDef("seq", (ProgramCounterType)-1),
	InstructionDef("end", 0),
	InstructionDef("q", 1),
	InstructionDef("cast", 2),
	InstructionDef("print", 1),
	InstructionDef("str", 1),
};

InstructionInfo get_instruction_info(std::string token);
InstructionInfo get_instruction_info(int index);
ProgramCounterType get_arg_count(InstructionDataType index);
