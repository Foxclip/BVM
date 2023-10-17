#include "program.h"
#include "utils.h"
#include "test.h"

void execute_program_debug(std::string path) {
	try {
		std::string program_text = utils::file_to_str(path);
		Program program(program_text);
		program.print_iterations = true;
		program.print_buffer_enabled = true;
		std::cout << "Nodes:";
		std::cout << "\n";
		program.print_nodes();
		std::vector<Token> results = program.execute();
		std::cout << "Results: ";
		program.print_tokens(program.tokens, false);
		std::cout << "Print buffer:\n";
		std::cout << program.global_print_buffer;
	} catch (std::exception exc) {
		throw std::runtime_error(path + ": " + exc.what());
	}
}

void execute_program_normal(std::string path) {
	try {
		std::string program_text = utils::file_to_str(path);
		Program program(program_text);
		program.print_buffer_enabled = true;
		std::vector<Token> results = program.execute();
	} catch (std::exception exc) {
		throw std::runtime_error(path + ": " + exc.what());
	}
}

int main() {
	try {

		//execute_program_debug("program.bvmi");
		//execute_program_normal("program.bvmi");
		test::run_tests();

	} catch (std::string msg) {
		std::cout << "ERROR: " << msg << "\n";
	} catch (std::exception exc) {
		std::cout << "ERROR: " << exc.what() << "\n";
	}

	// TODO: move instruction
	// TODO: mrep instruction
	// TODO: undeletable end instruction
	// movable list, seq, end instructions
	// TODO: duplicate label error, keyword label error, instruction name label error
	// TODO: duplicate test error
	// TODO: hanging files in test folder warning
	// TODO: string printing test
	// TODO: function call macro
	// TODO: nodes disappear if they are not connected to anything
	// TODO: wait instruction, like get but executes only if its target is a number
	// TODO: repl executes its second argument, also add code instruction (quote, written in source as ') to stop code from executing
	// TODO: move and swap instructions, for moving and swapping subtrees
	// TODO: def instruction, for defining macros
	// TODO: builtin macros (which are part of the intermediate language itself)
	// TODO: math functions take lists as arguments (and possibly other functions?)
	// TODO: adding numbers to lists and lists to numbers
	// TODO: make Token.str debug-only
	// TODO: block instruction, separates program into blocks, blocks have
	// TODO: replace command string tokens in the code with enum values
	// TODO: next and prev instructions, return address of the next and previous subtree
	// up, down to go between the tree levels
	// hshift and vshift to multiple steps
	// TODO: make parse_token function not recursive
	// TODO: do not parse code every time, keep parsed nodes around if they are not touched by modifying instructiions
	// TODO: shift pointers from pointer list, instead of scanning the whole token list
	// TODO: place program counter at the leftmost change position at new iteration
	// TODO: conv instruction, converts list of numbers to code
	// TODO: getsize instruction, returns size of the subtree
	// TODO: get instruction, returns two numbers: first number signifies whether token is an instruction or a number,
	// second number is num_value if it is a number, or instruction index if it is an instruction
	// TODO: strings as lists of numbers and output to stdout
	// TODO: file instruction that reads file and converts it into a list of chars
	// TODO: malloc, free and read instructions for working with heap memory
	// TODO: getaddr instruction, get absolute address of the current node
	// TODO: modifying instructions can only address stuff inside its list (block instruction?)
	// TODO: fractal lists?
	// TODO: parallel computation of lists
	// TODO: CUDA version

	return 0;
}