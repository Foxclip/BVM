#include "instruction.h"

InstructionInfo get_instruction_info(std::string token) {
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
}