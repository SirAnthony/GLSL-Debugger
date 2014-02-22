/*
 * debugchange.h
 *
 *  Created on: 20.10.2013
 */

#ifndef DEBUGCHANGE_H_
#define DEBUGCHANGE_H_

#include "traverse.h"
#include <set>
#include "ast.h"

class ast_debugchange_traverser_visitor: public ast_traverse_visitor {
public:
	ast_debugchange_traverser_visitor()
	{
		flags = traverse_debugvisit;
	}

	virtual ~ast_debugchange_traverser_visitor()
	{
	}

private:
	bool active;
};

class ir_debugchange_traverser_visitor: public ir_traverse_visitor {
public:
	ir_debugchange_traverser_visitor()
	{
		preVisit = false;
		postVisit = false;
		debugVisit = true;
	}

	virtual ~ir_debugchange_traverser_visitor()
	{
	}

	// Subclasses must implement this
	virtual bool visitIr(ir_variable *ir);
	virtual bool visitIr(ir_function_signature *ir);
	virtual bool visitIr(ir_expression *ir);
	virtual bool visitIr(ir_texture *ir);
	virtual bool visitIr(ir_swizzle *ir);
	virtual bool visitIr(ir_dereference_variable *ir);
	virtual bool visitIr(ir_dereference_array *ir);
	virtual bool visitIr(ir_dereference_record *ir);
	virtual bool visitIr(ir_assignment *ir);
	virtual bool visitIr(ir_constant *ir);
	virtual bool visitIr(ir_call *ir);
	virtual bool visitIr(ir_return *ir);
	virtual bool visitIr(ir_discard *ir);
	virtual bool visitIr(ir_if *ir);
	virtual bool visitIr(ir_loop *ir);
	virtual bool visitIr(ir_loop_jump *ir);
	virtual bool visitIr(ir_dummy *ir);

	bool isActive(void)
	{
		return active;
	}
	// active:  all coming symbols are being changed
	void activate(void)
	{
		active = true;
	}
	// passive: coming symbols act as input and are not changed
	void deactivate(void)
	{
		active = false;
	}

private:
	std::set<ir_function_signature*> parsed_signatures;
	bool active;
};

#endif /* DEBUGCHANGE_H_ */