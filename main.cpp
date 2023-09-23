#include "algorithm.h"
#include "utils.h"

int main() {
	try {
		std::string program_text = fileToStr("program.txt");
		Program program(program_text);
		program.parse();
		std::cout << "Nodes:";
		std::cout << "\n";
		program.print_nodes();
		std::vector<long> results = program.execute();
		std::cout << "Results: " << vector_to_str(results) << "\n";
	} catch (std::string msg) {
		std::cout << "EXCEPTION: " << msg << "\n";
	} catch (std::exception exc) {
		std::cout << "EXCEPTION: " << exc.what() << "\n";
	}

	// TODO: shift pointers from pointer list, instead of scanning the whole token list
	// TODO: repl instruction, replaces one subtree with another
	// TODO: replace command string tokens in the code with enum values
	// TODO: undeletable end instruction
	// TODO: move instruction, moves a node
	// TODO: conv instruction, converts list of numbers to code
	// TODO: getsize instruction, returns size of the list
	// TODO: get instruction, returns two numbers: first number signifies whether token is an instruction or a number,
	// second number is num_value if it is a number, or instruction index if it is an instruction
	// TODO: strings as lists of numbers and output to stdout
	// TODO: file instruction that reads file and converts it into a list of chars
	// TODO: malloc, free and read instructions for working with heap memory
	// TODO: getaddr instruction, get absolute address of the current node
	// TODO: modifying instructions can only address stuff inside its list
	// TODO: fractal lists?
	// TODO: parallel computation of lists and CUDA version

	return 0;
}