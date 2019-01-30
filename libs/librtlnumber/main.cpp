/* Authors: Aaron Graham (aaron.graham@unb.ca, aarongraham9@gmail.com),
 *           Jean-Philippe Legault (jlegault@unb.ca, jeanphilippe.legault@gmail.com) and
 *            Dr. Kenneth B. Kent (ken@unb.ca)
 *            for the Reconfigurable Computing Research Lab at the
 *             Univerity of New Brunswick in Fredericton, New Brunswick, Canada
 */

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "rtl_int.hpp"
#include "rtl_utils.hpp"

#define bad_ops(test) _bad_ops(test, __func__, __LINE__)
inline static std::string _bad_ops(std::string test, const char *FUNCT, int LINE)	
{	
	std::cerr << "INVALID INPUT OPS: (" << test << ")@" << FUNCT << "::" << std::to_string(LINE) << std::endl;	
	std::abort();
}

/***
 *     __   __       ___  __   __           ___       __       
 *    /  ` /  \ |\ |  |  |__) /  \ |       |__  |    /  \ |  | 
 *    \__, \__/ | \|  |  |  \ \__/ |___    |    |___ \__/ |/\| 
 *                                                             
 * 	This is used for testing purposes only, unused in ODIN as the input is already preprocessed
 */

static std::string arithmetic(std::string op, std::string a_in)
{
	DEBUG_MSG("Init:");DEBUG_NEWLINE();

	VNumber a(a_in);

	DEBUG_MSG("a: " << a.to_string());DEBUG_NEWLINE();
	
	/* return Process Operator via ternary */
	return (
		(op == "~")		?		(~a):
		(op == "-")		?		(-a):
		(op == "+")		?		(+a):
		(op == "&")		?		V_BITWISE_AND(a):
		(op == "|")		?		V_BITWISE_OR(a):
		(op == "^")		?		V_BITWISE_XOR(a):
		(op == "~&")	?		V_BITWISE_NAND(a):
		(op == "~|")	?		V_BITWISE_NOR(a):
		(op == "~^"	
		|| op == "^~")	?		V_BITWISE_XNOR(a):
		(op == "!")		?		(!a):
								bad_ops(op)
	).to_string();
}

static std::string arithmetic(std::string a_in, std::string op, std::string b_in)
{
	DEBUG_MSG("Init:");DEBUG_NEWLINE();

	VNumber a(a_in);
	VNumber b(b_in);

	DEBUG_MSG("a: " << a.to_string());DEBUG_NEWLINE();
	DEBUG_MSG("b: " << b.to_string());DEBUG_NEWLINE();

	/* return Process Operator via ternary */
	return (
		(op == "&")		?		(a & b):
		(op == "|")		?		(a | b):
		(op == "^")		?		(a ^ b):
		(op == "~&")	?		V_BITWISE_NAND(a, b):
		(op == "~|")	?		V_BITWISE_NOR(a, b):
		(op == "~^"	
		|| op == "^~")	?		V_BITWISE_XNOR(a, b):
		/*	Case test	*/
		(op == "===" )	?		V_CASE_EQUAL(a, b):
		(op == "!==")	?		V_CASE_NOT_EQUAL(a, b):
		/*	Shift Operator	*/
		(op == "<<")	?		(a << b):
		(op == "<<<")	?		V_SIGNED_SHIFT_LEFT(a, b):
		(op == ">>")	?		(a >> b):
		(op == ">>>")	?		V_SIGNED_SHIFT_RIGHT(a, b):
		/* Logical Operators */
		(op == "&&")	?		(a && b):
		(op == "||")	?		(a || b):
		(op == "<")		?		(a < b):																																													
		(op == ">")		?		(a > b):
		(op == "<=")	?		(a <= b):
		(op == ">=")	?		(a >= b):
		(op == "==")	?		(a == b):
		(op == "!=")	?		(a != b):
		/* arithmetic Operators */																
		(op == "+")		?		(a + b):
		(op == "-")		?		(a - b):
		(op == "*")		?		(a * b):
		(op == "**")	?		V_POWER(a, b):
		/* cannot div by 0 */
		(op == "/")		?		(a / b):
		(op == "%")		?		(a % b):
								bad_ops(op)
	).to_string();
}

static std::string arithmetic(std::string a_in, std::string op1 ,std::string b_in, std::string op2, std::string c_in)
{
	DEBUG_MSG("Init:");DEBUG_NEWLINE();

	VNumber a(a_in);
	VNumber b(b_in);
	VNumber c(c_in);

	DEBUG_MSG("a: " << a.to_string());DEBUG_NEWLINE();
	DEBUG_MSG("b: " << b.to_string());DEBUG_NEWLINE();
	DEBUG_MSG("c: " << c.to_string());DEBUG_NEWLINE();
	
	/* return Process Operator via ternary */
	return(	(op1 == "?" && op2 == ":")	?	V_TERNARY(a, b, c):
											bad_ops("?:")
	).to_string();
}

int main(int argc, char** argv) 
{
	DEBUG_NEWLINE();DEBUG_NEWLINE();DEBUG_MSG("Init:");DEBUG_NEWLINE();

	std::vector<std::string> input;
	for(int i=0; i < argc; i++)		input.push_back(argv[i]);

	std::string result = "";

	if(argc < 3)
	{
		ERR_MSG("Not Enough Arguments: " << std::to_string(argc - 1));

		DEBUG_MSG("End.");DEBUG_NEWLINE();

		return -1;
	}
	else if(argc == 3 && input[1] == "is_true")
	{
		DEBUG_MSG("input[1]: " << input[1]);DEBUG_NEWLINE();
		DEBUG_MSG("input[2]: " << input[2]);DEBUG_NEWLINE();

		VNumber input_2(input[2]);

		DEBUG_MSG("input_2: " << input_2.to_string());DEBUG_NEWLINE();
		DEBUG_MSG("Verify Result: Call V_TRUE(input_2):");DEBUG_NEWLINE();

		result = (V_TRUE(input_2) ? "pass" : "fail");
	}
	else if(argc == 3)
	{
		DEBUG_MSG("input[1]: " << input[1]);DEBUG_NEWLINE();
		DEBUG_MSG("input[2]: " << input[2]);DEBUG_NEWLINE();
		DEBUG_MSG("Unary: Call arithmetic(input[1], input[2]):");DEBUG_NEWLINE();

		result = arithmetic(input[1], input[2]);
	}
	else if(argc == 4)
	{
		DEBUG_MSG("input[1]: " << input[1]);DEBUG_NEWLINE();
		DEBUG_MSG("input[2]: " << input[2]);DEBUG_NEWLINE();
		DEBUG_MSG("input[3]: " << input[3]);DEBUG_NEWLINE();
		DEBUG_MSG("Binary: Call arithmetic(input[1], input[2], input[3]):");DEBUG_NEWLINE();

		result = arithmetic(input[1], input[2], input[3]);
	}
	else if(argc == 5)
	{
		// Binary or Ternary?
		ERR_MSG("Either Too Few (Ternary) or Too Many (Binary) Arguments: " << std::to_string(argc - 1));

		DEBUG_MSG("End.");DEBUG_NEWLINE();

		return -1;
	}
	else if(argc == 6)
	{
		DEBUG_MSG("input[1]: " << input[1]);DEBUG_NEWLINE();
		DEBUG_MSG("input[2]: " << input[2]);DEBUG_NEWLINE();
		DEBUG_MSG("input[3]: " << input[3]);DEBUG_NEWLINE();
		DEBUG_MSG("input[4]: " << input[4]);DEBUG_NEWLINE();
		DEBUG_MSG("input[5]: " << input[5]);DEBUG_NEWLINE();
		DEBUG_MSG("Ternary: Call arithmetic(input[1], input[2], input[3], input[4], input[5]):");DEBUG_NEWLINE();

		result = arithmetic(input[1], input[2], input[3], input[4], input[5]);
	}
	else				
	{
		ERR_MSG("Too Many Arguments: " << std::to_string(argc - 1));

		DEBUG_MSG("End.");DEBUG_NEWLINE();

		return -1;
	}

	DEBUG_MSG("result: " << result);DEBUG_NEWLINE();

	std::cout << result << std::endl;

	DEBUG_MSG("End.");DEBUG_NEWLINE();

	return 0;
}
