#include "algorithm.h"
#include "utils.h"
#include "test.h"

void execute_program(std::string path) {
	try {
		std::string program_text = utils::file_to_str(path);
		Program program(program_text);
		program.print_iterations = true;
		std::cout << "Nodes:";
		std::cout << "\n";
		program.print_nodes();
		std::vector<Token> results = program.execute();
		std::cout << "Results: ";
		program.print_tokens(false);
	} catch (std::exception exc) {
		throw std::runtime_error(path + ": " + exc.what());
	}
}

int main() {
	try {

		//execute_program("program.txt");
		//execute_program("tests/type_parse.txt");
		test::run_tests();

	} catch (std::string msg) {
		std::cout << "EXCEPTION: " << msg << "\n";
	} catch (std::exception exc) {
		std::cout << "EXCEPTION: " << exc.what() << "\n";
	}

	// TODO: rename repl to set and replp to repl
	// TODO: float numbers, int division by zero, float mod
	// TODO: adding number to pointer test
	// TODO: repl executes its second argument, also add code instruction to stop code from executing
	// TODO: adding numbers to lists and lists to numbers
	// TODO: block instruction, separates program into blocks, blocks have
	// TODO: functions can return pointers
	// TODO: replace command string tokens in the code with enum values
	// TODO: next and prev instructions, return address of the next and previous subtree
	// TODO: modifying instructions shift pointers in the tokens that are being moved around
	// if pointers lead inside then leave them as is, otherwise shift them so they point where they were pointing
	// TODO: shift pointers from pointer list, instead of scanning the whole token list
	// TODO: place program counter at the leftmost change position at new iteration
	// TODO: undeletable end instruction
	// TODO: move instruction, moves a node
	// TODO: do something with integer division by 0 (convert it to float and return inf?)
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
	// TODO: parallel computation of lists and CUDA version

	return 0;
}