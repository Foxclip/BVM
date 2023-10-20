#include "instruction.h"

bool operator<(const InstructionDef& left, const InstructionDef& right) {
	return left.str < right.str;
}

InstructionInfo get_instruction_info(std::string token) {
	try {
		auto it = std::find_if(INSTRUCTION_LIST.begin(), INSTRUCTION_LIST.end(),
			[&](InstructionDef def) {
				return def.str == token;
			}
		);
		if (it == INSTRUCTION_LIST.end()) {
			throw std::runtime_error("Instruction not found: " + token);
		}
		const InstructionDef def = *it;
		int index = it - INSTRUCTION_LIST.begin();
		InstructionInfo info(def.str, def.arg_count, index);
		return info;
	} catch (std::exception exc) {
		throw std::runtime_error(__FUNCTION__": " + std::string(exc.what()));
	}
}

InstructionInfo get_instruction_info(int index) {
	if (index < 0 || index >= INSTRUCTION_LIST.size()) {
		return InstructionInfo();
	}
	return InstructionInfo(INSTRUCTION_LIST[index], index);
}

InstructionDef get_instruction_def(std::string str) {
	try {
		auto it = INSTRUCTION_SET.find(InstructionDef(str, 0));
		if (it == INSTRUCTION_SET.end()) {
			throw std::runtime_error("Instruction not found: " + str);
		}
		return *it;
	} catch (std::exception exc) {
		throw std::runtime_error(__FUNCTION__": " + std::string(exc.what()));
	}
}

ProgramCounterType get_arg_count(InstructionDataType index) {
	return INSTRUCTION_LIST[index].arg_count;
}
