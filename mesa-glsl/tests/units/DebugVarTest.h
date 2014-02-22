/*
 * ast_debugvar.h
 *
 *  Created on: 12.02.2014
 */

#ifndef AST_DEBUGVAR_TEST_H_
#define AST_DEBUGVAR_TEST_H_

#include "ShaderInput.h"
#include "ShaderLang.h"
#include "glslang/Interface/Visitors/debugvar.h"
#include "Base.h"
#include <cppunit/TestSuite.h>
#include <cppunit/TestCaller.h>
#include <iomanip>

class DebugVarTest: public BaseUnitTest {
public:
	DebugVarTest()
	{
		unit_name = "dbgvar";
		std::string test_files = "shaders/test";
		holder = input.getShader(test_files);
		comparator.loadResults(test_files, unit_name);
	}

	void setUp()
	{
		vl = new ShVariableList;
		vl->numVariables = 0;
		vl->variables = NULL;
	}

	virtual void testShader(int num)
	{
		AstShader* sh = holder->shaders[num];
		if (!sh)
			return;
		ast_debugvar_traverser_visitor it(sh, vl);
		it.visit(sh->head);
		doComparison(sh);
	}

	static CppUnit::TestSuite *suite()
	{
		CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite;
		suiteOfTests->addTest(
				new CppUnit::TestCaller<DebugVarTest>("testVertex",
						&DebugVarTest::testVertex));
		suiteOfTests->addTest(
				new CppUnit::TestCaller<DebugVarTest>("testGeom",
						&DebugVarTest::testGeom));
		suiteOfTests->addTest(
				new CppUnit::TestCaller<DebugVarTest>("testFrag",
						&DebugVarTest::testFrag));
		return suiteOfTests;
	}

	virtual bool accept(int depth, ast_node* node, enum ast_node_type type)
	{
		int length = strlen(ast_node_names[type]);
		results << ast_node_names[type];

		ast_expression* expr = node->as_expression();
		if (expr) {
			const char* expr_type;
			if (expr->oper < ast_array_index)
				expr_type = expr->operator_string(expr->oper);
			else
				expr_type = ast_expr_string_ext[expr->oper - ast_array_index];
			results << " (" << expr_type << ")";
			length += strlen(expr_type) + 3;
		}

		if (!node->scope.is_empty()) {
			results << std::setfill(' ') << std::setw(40-length) << " ";
			for (int i = 0; i < depth; ++i)
				results << "    ";

			foreach_list(scope_node, &node->scope) {
				scope_item* sc = (scope_item*)scope_node;
				results << " <" << sc->id << "," << sc->name << ">";
			}
		}

		results << "\n";
		return true;
	}

	void tearDown()
	{
		delete vl;
	}

private:
	ShaderHolder* holder;
	ShVariableList* vl;
};

#endif /* AST_DEBUGVAR_TEST_H_ */