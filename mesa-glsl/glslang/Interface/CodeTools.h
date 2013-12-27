#ifndef _CODE_TOOLS_
#define _CODE_TOOLS_

#include "../Public/ShaderLang.h"
#include "../Include/Common.h"
#include "ir.h"

#define MAIN_FUNC_SIGNATURE "main"

struct exec_list;


__inline std::string FormatSourceRange(const YYLTYPE& range)
{
    char locText[128];

    sprintf(locText, "%4d:%3d - %4d:%3d", range.first_line, range.first_column,
                                        range.last_line, range.last_column);
    return std::string(locText);
}

std::string getMangledName(ir_function_signature* func);
char* getFunctionName(const char* in);
ir_function* getFunctionBySignature( const char *sig, struct gl_shader* shader );
int getFunctionDebugParameter(ir_function_signature *node);
ir_instruction* getIRDebugParameter(exec_list *list, int pnum);
ir_instruction* getSideEffectsDebugParameter(ir_call *ir, int pnum);

bool dbg_state_not_match( exec_list*, enum ir_dbg_state );
bool dbg_state_not_match( ir_dummy*, enum ir_dbg_state );

#endif
