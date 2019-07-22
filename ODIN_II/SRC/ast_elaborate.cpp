	/*
Copyright (c) 2009 Peter Andrew Jamieson (jamieson.peter@gmail.com)

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "odin_types.h"
#include "ast_util.h"
#include "ast_elaborate.h"
#include "ast_loop_unroll.h"
#include "parse_making_ast.h"
#include "verilog_bison.h"
#include "netlist_create_from_ast.h"
#include "ctype.h"
#include "vtr_util.h"
#include "vtr_memory.h"
#include <string>
#include <iostream>
#include <vector>
#include <stack>

#define read_node  1
#define write_node 2

#define e_data  1
#define e_operation 2
#define e_variable 3

#define N 64
#define Max_size 64

// long count_id = 0;
// long count_assign = 0;
// long count;
// long count_write;
//enode *head, *p;

// void reduce_assignment_expression();
// void reduce_assignment_expression(ast_node_t *ast_module);
// void find_assign_node(ast_node_t *node, std::vector<ast_node_t *> list, char *module_name);

// struct enode
// {
// 	struct
// 	{
// 		short operation;
// 		int data;
// 		std::string variable;
// 	}type;

// 	int id;
// 	int flag;
// 	int priority;
// 	struct enode *next;
// 	struct enode *pre;

// };

// void record_tree_info(ast_node_t *node);
// void store_exp_list(ast_node_t *node);
// void create_enode(ast_node_t *node);
// bool simplify_expression();
// bool adjoin_constant();
// enode *replace_enode(int data, enode *t, int mark);
// bool combine_constant();
// bool delete_continuous_multiply();
// void construct_new_tree(enode *tail, ast_node_t *node, int line_num, int file_num);
// void reduce_enode_list();
// enode *find_tail(enode *node);
// int check_exp_list(enode *tail);
// void create_ast_node(enode *temp, ast_node_t *node, int line_num, int file_num);
// void create_op_node(ast_node_t *node, enode *temp, int line_num, int file_num);
// void free_exp_list();
// void change_exp_list(enode *beign, enode *end, enode *s, int flag);
// enode *copy_enode_list(enode *new_head, enode *begin, enode *end);
// void copy_enode(enode *node, enode *new_node);
// bool check_tree_operation(ast_node_t *node);
// void check_operation(enode *begin, enode *end);
// bool check_mult_bracket(std::vector<int> list);

void remove_generate(ast_node_t *node);

void remove_generate(ast_node_t *node)
{
	for(int i=0; i<node->num_children; i++)
	{
		if(!node->children[i])
			continue;
		ast_node_t* candidate = node->children[i];
		ast_node_t* body_parent = nullptr;

		if(candidate->type == GENERATE)
		{
			for(int j = 0; j<candidate->num_children; j++)
			{
				ast_node_t* new_child = ast_node_deep_copy(candidate->children[j]);
				body_parent = body_parent ? newList_entry(body_parent, new_child) : newList(BLOCK, new_child);
			}
			node->children[i] = body_parent;
			free_whole_tree(candidate);
		} 
		else 
		{
			remove_generate(candidate);
		}
	}
}

int simplify_ast_module(ast_node_t **ast_module, STRING_CACHE_LIST *local_string_cache_list)
{
	/* resolve constant expressions */
	*ast_module = reduce_expressions(*ast_module, local_string_cache_list, NULL, 0);
	unroll_loops(ast_module, local_string_cache_list);
	remove_generate(*ast_module);

	return 1;
}

// this should replace ^^
ast_node_t *reduce_expressions(ast_node_t *node, STRING_CACHE_LIST *local_string_cache_list, long *max_size, long assignment_size)
{
	if (node)
	{
		STRING_CACHE *local_param_table_sc = local_string_cache_list->local_param_table_sc;
		STRING_CACHE *local_symbol_table_sc = local_string_cache_list->local_symbol_table_sc;

		switch (node->type)
		{
			case MODULE:
			{
				// skip identifier
				node->children[1] = reduce_expressions(node->children[1], local_string_cache_list, NULL, 0);
				node->children[2] = reduce_expressions(node->children[2], local_string_cache_list, NULL, 0);
				return node;
			}
			case FUNCTION:
			{
				return node;
			}
			case VAR_DECLARE:
			{
				if (node->types.variable.is_parameter || node->types.variable.is_localparam)
				{
					bool is_stored = false;
					long sc_spot = sc_lookup_string(local_param_table_sc, node->children[0]->types.identifier);
					if (sc_spot != -1 && ((ast_node_t*)local_param_table_sc->data[sc_spot]) == node->children[5])
					{
						is_stored = true;
					}

					/* resolve right-hand side */
					node->children[5] = reduce_expressions(node->children[5], local_string_cache_list, NULL, 0);
					oassert(node->children[5]->type == NUMBERS);

					/* this forces parameter values as unsigned, since we don't currently support signed keyword...
						must be changed once support is added */
					VNumber *temp = node->children[5]->types.vnumber;
					VNumber *to_unsigned = new VNumber(V_UNSIGNED(*temp));
					node->children[5]->types.vnumber = to_unsigned;
					delete temp;

					if (is_stored)
					{
						local_param_table_sc->data[sc_spot] = (void *) node->children[5];
					}

					return node;
				}
				break;
			}
			case IDENTIFIERS:
			{
				if (local_param_table_sc != NULL && node->types.identifier)
				{
					long sc_spot = sc_lookup_string(local_param_table_sc, node->types.identifier);
					if (sc_spot != -1)
					{
						ast_node_t *newNode = ast_node_deep_copy((ast_node_t *)local_param_table_sc->data[sc_spot]);

						if (newNode->type != NUMBERS)
						{
							newNode = reduce_expressions(newNode, local_string_cache_list, NULL, assignment_size);
							oassert(newNode->type == NUMBERS);

							/* this forces parameter values as unsigned, since we don't currently support signed keyword...
							must be changed once support is added */
							VNumber *temp = newNode->types.vnumber;
							VNumber *to_unsigned = new VNumber(V_UNSIGNED(*temp));
							newNode->types.vnumber = to_unsigned;
							delete temp;
							
							if (newNode->type != NUMBERS)
							{
								error_message(NETLIST_ERROR, node->line_number, node->file_number, "Parameter %s is not a constant expression\n", node->types.identifier);
							}
						}

						node = free_whole_tree(node);
						node = newNode;
					}
					else
					{
						break;
					}
				}
			}
			case FOR:
				// look ahead for parameters
				break;
			case WHILE:
				// look ahead for parameters
				break;
			case GENERATE:
				break;
			case BLOCKING_STATEMENT:
			case NON_BLOCKING_STATEMENT:
			{
				/* try to resolve */
				if (node->children[1]->type != FUNCTION_INSTANCE)
				{
					node->children[0] = reduce_expressions(node->children[0], local_string_cache_list, NULL, 0);

					assignment_size = get_size_of_variable(node->children[0], local_string_cache_list);
					max_size = (long*)calloc(1, sizeof(long));
					
					if (node->children[1]->type != NUMBERS)
					{
						node->children[1] = reduce_expressions(node->children[1], local_string_cache_list, max_size, assignment_size);
					}
					else
					{
						VNumber *temp = node->children[1]->types.vnumber;
						node->children[1]->types.vnumber = new VNumber(*temp, assignment_size);
						delete temp;
					}

					vtr::free(max_size);

					/* cast to unsigned if necessary */
					if (node_is_constant(node->children[1]))
					{
						char *id = NULL;
						if (node->children[0]->type == IDENTIFIERS)
						{
							id = node->children[0]->types.identifier;
						}
						else
						{
							id = node->children[0]->children[0]->types.identifier;
						}

						long sc_spot = sc_lookup_string(local_symbol_table_sc, id);
						if (sc_spot > -1)
						{
							bool is_signed = ((ast_node_t *)local_symbol_table_sc->data[sc_spot])->types.variable.is_signed;
							if (!is_signed)
							{
								VNumber *temp = node->children[1]->types.vnumber;
								VNumber *to_unsigned = new VNumber(V_UNSIGNED(*temp));
								node->children[1]->types.vnumber = to_unsigned;
								delete temp;
							}
							else
							{
								/* leave as is */
							}
						}
					}
					else
					{
						/* signed keyword is not supported, meaning unresolved values will already be handled as
							unsigned at the netlist level... must update once signed support is added */
					}
					
					assignment_size = 0;
				}

				return node;
			}
			case BINARY_OPERATION:
			case UNARY_OPERATION:
				break;
			case CONCATENATE:
				break;
			case REPLICATE:
				break;
			case NUMBERS:
				break;
			case IF_Q:
				break;
			case IF:
				break;
			case CASE:
				break;
			case RANGE_REF:
			case ARRAY_REF:
				break;
			case MODULE_INSTANCE:
				// flip hard blocks
				break;
			default:
				break;
		}

		/* recurse */
		for (int i = 0; i < node->num_children; i++)
		{
			node->children[i] = reduce_expressions(node->children[i], local_string_cache_list, max_size, assignment_size);
		}

		/* post-amble */
		switch (node->type)
		{
			case FOR:
				// unroll
				//unroll_loops(node->children[i]); // change this function (dont have to go through whole tree)
				// recurse for operation resolution
				break;
			case WHILE:
				// unroll
				// recurse (simplify_ast?)
			case GENERATE:
				/* remove unused node preventing module instantiation */
				//remove_generate(node->children[i]); // change this function (dont have to go through whole tree)
				break;
			case BINARY_OPERATION:
			{
				ast_node_t *new_node = fold_binary(&node);
				if (node_is_constant(new_node))
				{
					/* resize as needed */
					long new_size;
					long this_size = new_node->types.vnumber->size();

					if (assignment_size > 0)
					{
						new_size = assignment_size;
					}
					else if (max_size)
					{
						new_size = *max_size;
					}
					else
					{
						new_size = this_size;
					}

					/* clean up */
					free_resolved_children(node);
												
					change_to_number_node(node, VNumber(*(new_node->types.vnumber), new_size));
					new_node = free_whole_tree(new_node);
				}
				break;
			}
			case UNARY_OPERATION:
			{
				ast_node_t *new_node = fold_unary(&node);
				if (node_is_constant(new_node))
				{
					/* resize as needed */
					long new_size;
					long this_size = new_node->types.vnumber->size();

					if (assignment_size > 0)
					{
						new_size = assignment_size;
					}
					else if (max_size)
					{
						new_size = *max_size;
					}
					else
					{
						new_size = this_size;
					}

					/* clean up */
					free_resolved_children(node);
					
					change_to_number_node(node, VNumber(*(new_node->types.vnumber), new_size));
					new_node = free_whole_tree(new_node);
				}
				break;
			}
			case REPLICATE:
			{
				oassert(node_is_constant(node->children[0])); // should be taken care of in parse
				if( node->children[0]->types.vnumber->is_dont_care_string() )
				{
					error_message(NETLIST_ERROR, node->line_number, node->file_number, 
						"%s","Passing a non constant value to replication command, i.e. 2'bx1{...}");
				}

				int64_t value = node->children[0]->types.vnumber->get_value();
				if(value <= 0)
				{
					// todo, if this is part of a concat, it is valid
					error_message(NETLIST_ERROR, node->line_number, node->file_number, 
						"%s","Passing a number less than or equal to 0 for replication");
				}

				ast_node_t *new_node = create_node_w_type(CONCATENATE, node->line_number, node->file_number);
				for (size_t i = 0; i < value; i++)
				{
					add_child_to_node(new_node, ast_node_deep_copy(node->children[1]));
				}
				node = free_whole_tree(node);
				node = new_node;
			} 
			//fallthrough to resolve concatenation
			case CONCATENATE:
			{
				resolve_concat_sizes(node, local_string_cache_list);

				// for params only
				// TODO: this is a hack, concats cannot be folded in place as it breaks netlist expand from ast,
				// to fix we need to move the node resolution before netlist create from ast.
				if(node->num_children > 0)
				{
					size_t index = 1;
					size_t last_index = 0;
					
					while(index < node->num_children)
					{
						bool previous_is_constant = node_is_constant(node->children[last_index]);
						bool current_is_constant = node_is_constant(node->children[index]);

						if(previous_is_constant && current_is_constant)
						{
							VNumber new_value = V_CONCAT({*(node->children[last_index]->types.vnumber), *(node->children[index]->types.vnumber)});

							node->children[index] = free_whole_tree(node->children[index]);

							delete node->children[last_index]->types.vnumber;
							node->children[last_index]->types.vnumber = new VNumber(new_value);
						}
						else
						{
							last_index += 1;
							previous_is_constant = current_is_constant;
							node->children[last_index] = node->children[index];
						}
						index += 1;
					}

					node->num_children = last_index+1;

					if(node->num_children == 1)
					{
						ast_node_t *tmp = node->children[0];
						node->children[0] = NULL;
						free_whole_tree(node);
						node = tmp;
					}
				}

				break;
			}
			case IDENTIFIERS:
			{
				// look up to resolve unresolved range refs
				ast_node_t *var_node = NULL;

				oassert(node->types.identifier);
				long sc_spot = sc_lookup_string(local_symbol_table_sc, node->types.identifier);
				if (sc_spot > -1)
				{
					var_node = (ast_node_t *)local_symbol_table_sc->data[sc_spot];

					if (var_node->children[1] != NULL)
					{
						var_node->children[1] = reduce_expressions(var_node->children[1], local_string_cache_list, NULL, 0);
						var_node->children[2] = reduce_expressions(var_node->children[2], local_string_cache_list, NULL, 0);
					}
					if (var_node->children[3] != NULL)
					{
						var_node->children[3] = reduce_expressions(var_node->children[3], local_string_cache_list, NULL, 0);
						var_node->children[4] = reduce_expressions(var_node->children[4], local_string_cache_list, NULL, 0);
					}
					if (var_node->num_children == 8 && var_node->children[5])
					{
						var_node->children[5] = reduce_expressions(var_node->children[5], local_string_cache_list, NULL, 0);
						var_node->children[6] = reduce_expressions(var_node->children[6], local_string_cache_list, NULL, 0);
					}

					local_symbol_table_sc->data[sc_spot] = (void *)var_node;
				}

				if (max_size)
				{
					long var_size = get_size_of_variable(node, local_string_cache_list);
					if (var_size > *max_size)
					{
						*max_size = var_size;
					}
				}

				break;
			}
			case NUMBERS:
			{
				if (max_size)
				{
					if (node->types.vnumber->size() > (*max_size))
					{
						*max_size = node->types.vnumber->size();
					}
				}

				break;
			}
			case IF_Q:
				break;
			case IF:
				break;
			case CASE:
				break;
			case RANGE_REF:
			case ARRAY_REF:
				break;
			case MODULE_INSTANCE:
				// flip hard blocks
				break;
			default:
				break;
		}
	}

	return node;
}

// /*---------------------------------------------------------------------------
//  * (function: reduce_assignment_expression)
//  * reduce the number nodes which can be calculated to optimize the AST
//  *-------------------------------------------------------------------------*/
// void reduce_assignment_expression(ast_node_t *ast_module)
// {
// 	head = NULL;
// 	p = NULL;
// 	ast_node_t *T = NULL;

// 	if (count_id < ast_module->unique_count)
// 		count_id = ast_module->unique_count;

// 	count_assign = 0;
// 	std::vector<ast_node_t *> list_assign;
// 	find_assign_node(ast_module, list_assign, ast_module->children[0]->types.identifier);
// 	for (long j = 0; j < count_assign; j++)
// 	{
// 		if (check_tree_operation(list_assign[j]->children[1]) && (list_assign[j]->children[1]->num_children > 0))
// 		{
// 			store_exp_list(list_assign[j]->children[1]);

// 			if (simplify_expression())
// 			{
// 				enode *tail = find_tail(head);
// 				free_whole_tree(list_assign[j]->children[1]);
// 				T = (ast_node_t*)vtr::malloc(sizeof(ast_node_t));
// 				construct_new_tree(tail, T, list_assign[j]->line_number, list_assign[j]->file_number);
// 				list_assign[j]->children[1] = T;
// 			}
// 			free_exp_list();
// 		}
// 	}
// }

// /*---------------------------------------------------------------------------
//  * (function: search_assign_node)
//  * find the top of the assignment expression
//  *-------------------------------------------------------------------------*/
// void find_assign_node(ast_node_t *node, std::vector<ast_node_t *> list, char *module_name)
// {
// 	if (node && node->num_children > 0 && (node->type == BLOCKING_STATEMENT || node->type == NON_BLOCKING_STATEMENT))
// 	{
// 		list.push_back(node);
// 	}

// 	for (long i = 0; node && i < node->num_children; i++)
// 		find_assign_node(node->children[i], list, module_name);
// }

// /*---------------------------------------------------------------------------
//  * (function: simplify_expression)
//  * simplify the expression stored in the linked_list
//  *-------------------------------------------------------------------------*/
// bool simplify_expression()
// {
// 	bool build = adjoin_constant();
// 	reduce_enode_list();

// 	if(delete_continuous_multiply())
// 		build = true;

// 	if(combine_constant())
// 		build = true;

// 	reduce_enode_list();
// 	return build;
// }

// /*---------------------------------------------------------------------------
//  * (function: find_tail)
//  *-------------------------------------------------------------------------*/
// enode *find_tail(enode *node)
// {
// 	enode *temp;
// 	enode *tail = NULL;
// 	for (temp = node; temp != NULL; temp = temp->next)
// 		if (temp->next == NULL)
// 			tail = temp;

// 	return tail;
// }

// /*---------------------------------------------------------------------------
//  * (function: reduce_enode_list)
//  *-------------------------------------------------------------------------*/
// void reduce_enode_list()
// {
// 	enode *temp;
// 	int a;

// 	while(head != NULL && (head->type.data == 0) && (head->next->priority == 2) && (head->next->type.operation == '+')){
// 		temp=head;
// 		head = head->next->next;
// 		head->pre = NULL;

// 		vtr::free(temp->next);
// 		vtr::free(temp);
// 	}

// 	if(head == NULL){
// 		return;
// 	}

// 	temp = head->next;
// 	while (temp != NULL)
// 	{
// 		if ((temp->flag == 1) && (temp->pre->priority == 2))
// 		{
// 			if (temp->type.data == 0)
// 			{
// 				if (temp->next == NULL)
// 					temp->pre->pre->next = NULL;
// 				else
// 				{
// 					temp->pre->pre->next = temp->next;
// 					temp->next->pre = temp->pre->pre;
// 				}
// 				vtr::free(temp->pre);

// 				enode *toBeDeleted = temp;
// 				temp = temp->next;
// 				vtr::free(toBeDeleted);
// 			} else if (temp->type.data < 0)
// 			{
// 				if (temp->pre->type.operation == '+')
// 					temp->pre->type.operation = '-';
// 				else
// 					temp->pre->type.operation = '+';
// 				a = temp->type.data;
// 				temp->type.data = -a;

// 				temp = temp->next;
// 			} else {
// 				temp = temp->next;
// 			}
// 		} else {
// 			temp = temp->next;
// 		}
// 	}
// }

// /*---------------------------------------------------------------------------
//  * (function: store_exp_list)
//  *-------------------------------------------------------------------------*/
// void store_exp_list(ast_node_t *node)
// {
// 	enode *temp;
// 	head = (enode*)vtr::malloc(sizeof(enode));
// 	p = head;
// 	record_tree_info(node);
// 	temp = head;
// 	head = head->next;
// 	head->pre = NULL;
// 	p->next = NULL;
// 	vtr::free(temp);

// }

// /*---------------------------------------------------------------------------
//  * (function: record_tree_info)
//  *-------------------------------------------------------------------------*/
// void record_tree_info(ast_node_t *node)
// {
// 	if (node){
// 		if (node->num_children > 0)
// 		{
// 			record_tree_info(node->children[0]);
// 			create_enode(node);
// 			if (node->num_children > 1)
// 				record_tree_info(node->children[1]);
// 		}else{
// 			create_enode(node);
// 		}
// 	}
// }

// /*---------------------------------------------------------------------------
//  * (function: create_enode)
//  * store elements of an expression in nodes consisting the linked_list
//  *-------------------------------------------------------------------------*/
// void create_enode(ast_node_t *node)
// {
// 	enode *s = (enode*)vtr::malloc(sizeof(enode));
// 	s->flag = -1;
// 	s->priority = -1;
// 	s->id = node->unique_count;
	
// 	switch (node->type)
// 	{
// 		case NUMBERS:
// 		{
// 			s->type.data = node->types.vnumber->get_value();
// 			s->flag = 1;
// 			s->priority = 0;
// 			break;
// 		}
// 		case BINARY_OPERATION:
// 		{
// 			s->flag = 2;
// 			switch(node->types.operation.op)
// 			{
// 				case ADD:
// 				{
// 					s->type.operation = '+';
// 					s->priority = 2;
// 					break;
// 				}
// 				case MINUS:
// 				{
// 					s->type.operation = '-';
// 					s->priority = 2;
// 					break;
// 				}
// 				case MULTIPLY:
// 				{
// 					s->type.operation = '*';
// 					s->priority = 1;
// 					break;
// 				}
// 				case DIVIDE:
// 				{
// 					s->type.operation = '/';
// 					s->priority = 1;
// 					break;
// 				}
// 				default:
// 				{
// 					break;
// 				}
// 			}
// 			break;
// 		}
// 		case IDENTIFIERS:
// 		{
// 			s->type.variable = node->types.identifier;
// 			s->flag = 3;
// 			s->priority = 0;
// 			break;
// 		}
// 		default:
// 		{
// 			break;
// 		}
// 	}

// 	p->next = s;
// 	s->pre = p;
// 	p = s;

// }

// /*---------------------------------------------------------------------------
//  * (function: adjoin_constant)
//  * compute the constant numbers in the linked_list
//  *-------------------------------------------------------------------------*/
// bool adjoin_constant()
// {
// 	bool build = false;
// 	enode *t, *replace;
// 	int a, b, result = 0;
// 	int mark;
// 	for (t = head; t->next!= NULL; )
// 	{
// 		mark = 0;
// 		if ((t->flag == 1) && (t->next->next != NULL) && (t->next->next->flag ==1))
// 		{
// 			switch (t->next->priority)
// 			{
// 				case 1:
// 					a = t->type.data;
// 					b = t->next->next->type.data;
// 					if (t->next->type.operation == '*')
// 						result = a * b;
// 					else
// 						result = a / b;
// 					replace = replace_enode(result, t, 1);
// 					build = true;
// 					mark = 1;
// 					break;

// 				case 2:
// 					if (((t == head) || (t->pre->priority == 2)) && ((t->next->next->next == NULL) ||(t->next->next->next->priority ==2)))
// 					{
// 						a = t->type.data;
// 						b = t->next->next->type.data;
// 						if (t->pre->type.operation == '+')
// 						{
// 							if (t->next->type.operation == '+')
// 								result = a + b;
// 							else
// 								result = a - b;
// 						}
// 						if (t->pre->type.operation == '-')
// 						{
// 							if (t->next->type.operation == '+')
// 								result = a - b;
// 							else
// 								result = a + b;
// 						}

// 						replace = replace_enode(result, t, 1);
// 						build = true;
// 						mark = 1;
// 					}
// 					break;
//                 default:
//                     //pass
//                     break;
// 			}
// 		}
// 		if (mark == 0)
// 			t = t->next;
// 		else
// 			if (mark == 1)
// 				t = replace;

// 	}
// 	return build;
// }

// /*---------------------------------------------------------------------------
//  * (function: replace_enode)
//  *-------------------------------------------------------------------------*/
// enode *replace_enode(int data, enode *t, int mark)
// {
// 	enode *replace;
// 	replace = (enode*)vtr::malloc(sizeof(enode));
// 	replace->type.data = data;
// 	replace->flag = 1;
// 	replace->priority = 0;

// 	if (t == head)
// 	{
// 		replace->pre = NULL;
// 		head = replace;
// 	}

// 	else
// 	{
// 		replace->pre = t->pre;
// 		t->pre->next = replace;
// 	}

// 	if (mark == 1)
// 	{
// 		replace->next = t->next->next->next;
// 		if (t->next->next->next == NULL)
// 			replace->next = NULL;
// 		else
// 			t->next->next->next->pre = replace;
// 		vtr::free(t->next->next);
// 		vtr::free(t->next);
// 	}
// 	else
// 	{
// 		replace->next = t->next;
// 		t->next->pre = replace;
// 	}

// 	vtr::free(t);
// 	return replace;

// }

// /*---------------------------------------------------------------------------
//  * (function: combine_constant)
//  *-------------------------------------------------------------------------*/
// bool combine_constant()
// {
// 	enode *temp, *m, *s1, *s2, *replace;
// 	int a, b, result;
// 	int mark;
// 	bool build = false;

// 	for (temp = head; temp != NULL; )
// 	{
// 		mark = 0;
// 		if ((temp->flag == 1) && (temp->next != NULL) && (temp->next->priority == 2))
// 		{
// 			if ((temp == head) || (temp->pre->priority == 2))
// 			{
// 				for (m = temp->next; m != NULL; m = m->next)
// 				{
// 					if((m->flag == 1) && (m->pre->priority == 2) && ((m->next == NULL) || (m->next->priority == 2)))
// 					{
// 						s1 = temp;
// 						s2 = m;
// 						a = s1->type.data;
// 						b = s2->type.data;
// 						if ((s1 == head) || (s1->pre->type.operation == '+'))
// 						{
// 							if (s2->pre->type.operation == '+')
// 								result = a + b;
// 							else
// 								result = a - b;
// 						}
// 						else
// 						{
// 							if (s2->pre->type.operation == '+')
// 								result = a - b;
// 							else
// 								result = a + b;
// 						}
// 						replace = replace_enode(result, s1, 2);
// 						if (s2->next == NULL)
// 						{
// 							s2->pre->pre->next = NULL;
// 							mark = 2;
// 						}
// 						else
// 						{
// 							s2->pre->pre->next = s2->next;
// 							s2->next->pre = s2->pre->pre;
// 						}

// 						vtr::free(s2->pre);
// 						vtr::free(s2);
// 						if (replace == head)
// 						{
// 							temp = replace;
// 							mark = 1;
// 						}
// 						else
// 							temp = replace->pre;
// 						build = true;

// 						break;
// 					}
// 				}
// 			}
// 		}
// 		if (mark == 0)
// 			temp = temp->next;
// 		else
// 			if (mark == 1)
// 				continue;
// 			else
// 				break;
// 	}
// 	return build;

// }

// /*---------------------------------------------------------------------------
//  * (function: construct_new_tree)
//  *-------------------------------------------------------------------------*/
// void construct_new_tree(enode *tail, ast_node_t *node, int line_num, int file_num)
// {
// 	enode *temp, *tail1 = NULL, *tail2 = NULL;
// 	int prio = 0;

// 	if (tail == NULL)
// 		return;

// 	prio = check_exp_list(tail);
// 	if (prio == 1)
// 	{
// 		for (temp = tail; temp != NULL; temp = temp->pre)
// 			if ((temp->flag == 2) && (temp->priority == 2))
// 			{
// 				create_ast_node(temp, node, line_num, file_num);
// 				tail1 = temp->pre;
// 				tail2 = find_tail(temp->next);
// 				tail1->next = NULL;
// 				temp->next->pre = NULL;
// 				break;
// 			}

// 	}
// 	else
// 		if(prio == 2)
// 		{
// 			for (temp = tail; temp != NULL; temp = temp->pre)
// 				if ((temp->flag ==2) && (temp->priority == 1))
// 				{
// 					create_ast_node(temp, node, line_num, file_num);
// 					tail1 = temp->pre;
// 					tail2 = temp->next;
// 					tail2->pre = NULL;
// 					tail2->next = NULL;
// 					break;
// 				}
// 		}
// 		else
// 			if (prio == 3)
// 				create_ast_node(tail, node, line_num, file_num);

// 	if (prio == 1 || prio == 2)
// 	{
// 		node->children = (ast_node_t**)vtr::malloc(2*sizeof(ast_node_t*));
// 		node->children[0] = (ast_node_t*)vtr::malloc(sizeof(ast_node_t));
// 		node->children[1] = (ast_node_t*)vtr::malloc(sizeof(ast_node_t));

// 		create_ast_node(tail1, node->children[0], line_num, file_num);
// 		create_ast_node(tail2, node->children[1], line_num, file_num);
		
// 		construct_new_tree(tail1, node->children[0], line_num, file_num);
// 		construct_new_tree(tail2, node->children[1], line_num, file_num);
// 	}

// 	return;
// }

// /*---------------------------------------------------------------------------
//  * (function: check_exp_list)
//  *-------------------------------------------------------------------------*/
// int check_exp_list(enode *tail)
// {
// 	enode *temp;
// 	for (temp = tail; temp != NULL; temp = temp->pre)
// 		if ((temp->flag == 2) && (temp->priority == 2))
// 			return 1;

// 	for (temp = tail; temp != NULL; temp = temp->pre)
// 		if ((temp->flag == 2) && (temp->priority ==1))
// 			return 2;

// 	return 3;
// }

// /*---------------------------------------------------------------------------
//  * (function: delete_continuous_multiply)
//  *-------------------------------------------------------------------------*/
// bool delete_continuous_multiply()
// {
// 	enode *temp, *t, *m, *replace;
// 	bool build = false;
// 	int a, b, result;
// 	int mark;
// 	for (temp = head; temp != NULL; )
// 	{
// 		mark = 0;
// 		if ((temp->flag == 1) && (temp->next != NULL) && (temp->next->priority == 1))
// 		{
// 			for (t = temp->next; t != NULL; t = t->next)
// 			{
// 				if ((t->flag == 1) && (t->pre->priority ==1))
// 				{
// 					for (m = temp->next; m != t; m = m->next)
// 					{
// 						if ((m->flag == 2) && (m->priority != 1))
// 						{
// 							mark = 1;
// 							break;
// 						}

// 					}
// 					if (mark == 0)
// 					{
// 						a = temp->type.data;
// 						b = t->type.data;
// 						if (t->pre->type.operation == '*')
// 							result = a * b;
// 						else
// 							result = a / b;
// 						replace = replace_enode(result, temp, 3);
// 						if (t->next == NULL)
// 							t->pre->pre->next = NULL;
// 						else
// 						{
// 							t->pre->pre->next = t->next;
// 							t->next->pre = t->pre->pre;
// 						}
// 						mark = 2;
// 						build = true;
// 						vtr::free(t->pre);
// 						vtr::free(t);
// 						break;
// 					}
// 					break;
// 				}
// 			}
// 		}
// 		if ((mark == 0) || (mark == 1))
// 			temp = temp->next;
// 		else
// 			if (mark == 2)
// 				temp = replace;

// 	}
// 	return build;
// }

// /*---------------------------------------------------------------------------
//  * (function: create_ast_node)
//  *-------------------------------------------------------------------------*/
// void create_ast_node(enode *temp, ast_node_t *node, int line_num, int file_num)
// {
// 	switch(temp->flag)
// 	{
// 		case 1:
// 			initial_node(node, NUMBERS, line_num, file_num, ++count_id);
// 			change_to_number_node(node, temp->type.data);
// 		break;

// 		case 2:
// 			create_op_node(node, temp, line_num, file_num);
// 		break;

// 		case 3:
// 			initial_node(node, IDENTIFIERS, line_num, file_num, ++count_id);
// 			node->types.identifier = vtr::strdup(temp->type.variable.c_str());
// 		break;

// 		default:
// 		break;
// 	}

// }

// /*---------------------------------------------------------------------------
//  * (function: create_op_node)
//  *-------------------------------------------------------------------------*/
// void create_op_node(ast_node_t *node, enode *temp, int line_num, int file_num)
// {
// 	initial_node(node, BINARY_OPERATION, line_num, file_num, ++count_id);
// 	node->num_children = 2;
// 	switch(temp->type.operation)
// 	{
// 		case '+':
// 			node->types.operation.op = ADD;
// 		break;

// 		case '-':
// 			node->types.operation.op = MINUS;
// 		break;

// 		case '*':
// 			node->types.operation.op = MULTIPLY;
// 		break;

// 		case '/':
// 			node->types.operation.op = DIVIDE;
// 		break;
//         default:
//             oassert(false);
//         break;
// 	}

// }

// /*---------------------------------------------------------------------------
//  * (function: free_exp_list)
//  *-------------------------------------------------------------------------*/
// void free_exp_list()
// {
// 	enode *next, *temp;
// 	for (temp = head; temp != NULL; temp = next)
// 	{
// 		next = temp->next;
// 		vtr::free(temp);
// 	}
// }

// /*---------------------------------------------------------------------------
//  * (function: change_exp_list)
//  *-------------------------------------------------------------------------*/
// void change_exp_list(enode *begin, enode *end, enode *s, int flag)
// {
// 	enode *temp, *new_head, *tail, *p2, *partial = NULL, *start = NULL;
// 	int mark;
// 	switch (flag)
// 	{
// 		case 1:
// 		{
// 			for (temp = begin; temp != end; temp = p2->next)
// 			{
// 				mark = 0;
// 				for (p2 = temp; p2 != end->next; p2 = p2->next)
// 				{
// 					if (p2 == end)
// 					{
// 						mark = 2;
// 						break;
// 					}
// 					if ((p2->flag == 2) && (p2->priority == 2))
// 					{
// 						partial = p2->pre;
// 						mark = 1;
// 						break;
// 					}
// 				}
// 				if (mark == 1)
// 				{
// 					new_head = (enode*)vtr::malloc(sizeof(enode));
// 					tail = copy_enode_list(new_head, end->next, s);
// 					tail = tail->pre;
// 					vtr::free(tail->next);
// 					partial->next = new_head;
// 					new_head->pre = partial;
// 					tail->next = p2;
// 					p2->pre = tail;
// 				}
// 				if (mark == 2)
// 					break;
// 			}
// 			break;
// 		}

// 		case 2:
// 		{
// 			for (temp = begin; temp != end->next; temp = temp->next)
// 				if ((temp->flag == 2) && (temp->priority == 2))
// 				{
// 					start = temp;
// 					break;
// 				}
// 			for (temp = start; temp != end->next; temp = partial->next)
// 			{
// 				mark = 0;
// 				for (p2 = temp; p2 != end->next; p2 = p2->next)
// 				{
// 					if (p2 == end)
// 					{
// 						mark = 2;
// 						break;
// 					}
// 					if ((p2->flag == 2) && (p2->priority == 2))
// 					{
// 						partial = p2->next;
// 						mark = 1;
// 						break;
// 					}
// 				}
// 				if (mark == 1)
// 				{
// 					new_head = (enode*)vtr::malloc(sizeof(enode));
// 					tail = copy_enode_list(new_head, s, begin->pre);
// 					tail = tail->pre;
// 					vtr::free(tail->next);
// 					p2->next = new_head;
// 					new_head->pre = p2;
// 					tail->next = partial;
// 					partial->pre = tail;

// 				}
// 				if (mark == 2)
// 					break;
// 			}
// 			break;
// 		}
//         default:
//             break;

// 	}
// }

// /*---------------------------------------------------------------------------
//  * (function: copy_enode_list)
//  *-------------------------------------------------------------------------*/
// enode *copy_enode_list(enode *new_head, enode *begin, enode *end)
// {
// 	enode *temp, *new_enode, *next_enode;
// 	new_enode = new_head;
// 	for (temp = begin; temp != end->next; temp = temp->next)
// 	{
// 		copy_enode(temp, new_enode);
// 		next_enode = (enode*)vtr::malloc(sizeof(enode));
// 		new_enode->next = next_enode;
// 		next_enode->pre = new_enode;
// 		new_enode = next_enode;
// 	}

// 	return new_enode;
// }

// /*---------------------------------------------------------------------------
//  * (function: copy_enode)
//  *-------------------------------------------------------------------------*/
// void copy_enode(enode *node, enode *new_node)
// {
// 	new_node->type = node->type;
// 	new_node->flag = node->flag;
// 	new_node->priority = node->priority;
// 	new_node->id = -1;
// }

// /*---------------------------------------------------------------------------
//  * (function: check_tree_operation)
//  *-------------------------------------------------------------------------*/
// bool check_tree_operation(ast_node_t *node)
// {
// 	if (node && (node->type == BINARY_OPERATION) && ((node->types.operation.op == ADD ) || (node->types.operation.op == MINUS) || (node->types.operation.op == MULTIPLY)))
// 		return true;

// 	for (long i = 0; node &&  i < node->num_children; i++)
// 		if(check_tree_operation(node->children[i]))
// 			return true;

// 	return false;
// }

// /*---------------------------------------------------------------------------
//  * (function: check_operation)
//  *-------------------------------------------------------------------------*/
// void check_operation(enode *begin, enode *end)
// {
// 	enode *temp, *op;
// 	for (temp = begin; temp != head; temp = temp->pre)
// 	{
// 		if (temp->flag == 2 && temp->priority == 2 && temp->type.operation == '-')
// 		{
// 			for (op = begin; op != end; op = op->next)
// 			{
// 				if (op->flag == 2 && op->priority == 2)
// 				{
// 					if (op->type.operation == '+')
// 						op->type.operation = '-';
// 					else
// 						op->type.operation = '+';
// 				}
// 			}
// 		}
// 	}
// }
