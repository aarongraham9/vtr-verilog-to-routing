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

	VNumber a(a_in);

	
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

	VNumber a(a_in);
	VNumber b(b_in);


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

	VNumber a(a_in);
	VNumber b(b_in);
	VNumber c(c_in);

	
	/* return Process Operator via ternary */
	return(	(op1 == "?" && op2 == ":")	?	V_TERNARY(a, b, c):
											bad_ops("?:")
	).to_string();
}

int main(int argc, char** argv) 
{

	std::vector<std::string> input;
	for(int i=0; i < argc; i++)		input.push_back(argv[i]);

	std::string result = "";

	if(argc < 3)
	{
		ERR_MSG("Not Enough Arguments: " << std::to_string(argc - 1));


		return -1;
	}
	else if(argc == 3 && input[1] == "is_true")
	{

		VNumber input_2(input[2]);


		result = (V_TRUE(input_2) ? "pass" : "fail");
	}
	else if(argc == 3)
	{

		result = arithmetic(input[1], input[2]);
	}
	else if(argc == 4)
	{

		result = arithmetic(input[1], input[2], input[3]);
	}
	else if(argc == 5)
	{
		// Binary or Ternary?
		ERR_MSG("Either Too Few (Ternary) or Too Many (Binary) Arguments: " << std::to_string(argc - 1));


		return -1;
	}
	else if(argc == 6)
	{

		result = arithmetic(input[1], input[2], input[3], input[4], input[5]);
	}
	else				
	{
		ERR_MSG("Too Many Arguments: " << std::to_string(argc - 1));


		return -1;
	}


	std::cout << result << std::endl;


	return 0;
}
