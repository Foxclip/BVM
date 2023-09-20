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

	// TODO: replace command string tokens in the code with enum values
	// TODO: fractal lists?
	// TODO: strings as lists of numbers and output to stdout
	// TODO: modifying instructions can only address stuff inside its list
	// TODO: file instruction that reads file and converts it into a list of chars
	// TODO: move instruction, moves a node
	// TODO: getaddr instruction, get address of the current node
	// TODO: get instruction, returns two numbers: first number signifies whether token is an instruction or a number,
	// second number is num_value if it is a number, or instruction index if it is an instruction

	return 0;
}