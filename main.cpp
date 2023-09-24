#include "algorithm.h"
#include "utils.h"
#include "test.h"

void execute_program(std::string path) {
	std::string program_text = fileToStr(path);
	Program program(program_text);
	program.print_iterations = true;
	std::cout << "Nodes:";
	std::cout << "\n";
	program.print_nodes();
	std::vector<long> results = program.execute();
	std::cout << "Results: " << vector_to_str(results) << "\n";
}

int main() {
	try {

		//execute_program("program.txt");
		test::run_tests("tests/");

	} catch (std::string msg) {
		std::cout << "EXCEPTION: " << msg << "\n";
	} catch (std::exception exc) {
		std::cout << "EXCEPTION: " << exc.what() << "\n";
	}

	// TODO: make unit tests
	// TODO: repl instruction, replaces one subtree with another
	// TODO: functions can return pointers
	// TODO: replace command string tokens in the code with enum values
	// TODO: shift pointers from pointer list, instead of scanning the whole token list
	// TODO: place program counter at the leftmost change position at new iteration
	// TODO: undeletable end instruction
	// TODO: move instruction, moves a node
	// TODO: float type
	// TODO: do something with integer division by 0 (convert it to float and return inf?)
	// TODO: conv instruction, converts list of numbers to code
	// TODO: getsize instruction, returns size of the subtree
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