
find_package(Perl REQUIRED)


set(GENERATOR_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(MAKE_DIRECTORY ${GENERATOR_OUTPUT_DIR})

function(generate_file input output)
    execute_process(
        COMMAND ${PERL_EXECUTABLE} ${input} -p ${GENERATOR_OUTPUT_DIR} ${ARGN}
        WORKING_DIRECTORY ${GENERATOR_DIR}
        OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/${output}
    )
endfunction()

# Read gl and build api definition
message(STATUS "Reading gl headers")
generate_file(glreader.pl glheaders.pm)

# Generate enumerants
message(STATUS "Generate enumerants")
generate_file(Enumerants.pl glenumerants.h)

# Generate generator for list of allowed functions between begin/end
message(STATUS "Generate allowed function list generator.")
generate_file(BeginEndFunctionTest.pl beginEndFunctionTest.c)
generate_file(mergeAllowedInBeginEndList.pl functionsAllowedInBeginEnd.pm)

# Generate debug library
message(STATUS "Generate debug library")
generate_file(FunctionRef.pl functionRefs.inc)
generate_file(GetProcAddressHook.pl getProcAddressHook.inc)
generate_file(FunctionPointerTypes.pl functionPointerTypes.inc)
generate_file(streamHintRef.pl streamHintRefs.inc)
generate_file(ReplayFunc.pl replayFunction.c)
generate_file(FunctionList.pl functionList.c)
generate_file(FunctionHooks.pl functionHooks.inc)

if(GLSLDB_WIN)
    # Generate windows trampolines
    message(STATUS "Generate trampolines")
	generate_file(Trampolines.pl trampolines.h -mdecl)
	generate_file(Trampolines.pl trampolines.inc -mdef)
	generate_file(Trampolines.pl glsldebug.def -mexp)
endif()
