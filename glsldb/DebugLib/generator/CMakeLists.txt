
find_package(Perl REQUIRED)


set(GENERATOR_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(MAKE_DIRECTORY ${GENERATOR_OUTPUT_DIR})

function(generate_file input output)
    execute_process(
        COMMAND ${PERL_EXECUTABLE} ${input} -p ${GENERATOR_OUTPUT_DIR}
        WORKING_DIRECTORY ${GENERATOR_DIR}
        OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/${output}
    )
endfunction()

# Read gl and build api definition
message(STATUS "Reading gl headers")
execute_process(COMMAND ${PERL_EXECUTABLE}
    WORKING_DIRECTORY ${GENERATOR_DIR}
    INPUT_FILE glreader.pl
    OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/glheaders.pm
)

# Generate enumerants
message(STATUS "Generate enumerants")
generate_file(Enumerants.pl glenumerants.h)

# Generate generator for list of allowed functions between begin/end
message(STATUS "Generate allowed function list generator.")
generate_file(BeginEndFunctionTest.pl beginEndFunctionTest.c)
execute_process(COMMAND ${PERL_EXECUTABLE}
    WORKING_DIRECTORY ${GENERATOR_DIR}
    INPUT_FILE mergeAllowedInBeginEndList.pl
    OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/functionsAllowedInBeginEnd.pm
)

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
    execute_process(COMMAND ${PERL_EXECUTABLE} Trampolines.pl decl
        WORKING_DIRECTORY ${GENERATOR_DIR}
        OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/trampolines.h
    )
    execute_process(COMMAND ${PERL_EXECUTABLE} Trampolines.pl def
        WORKING_DIRECTORY ${GENERATOR_DIR}
        OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/trampolines.inc
    )
    execute_process(COMMAND ${PERL_EXECUTABLE} Trampolines.pl exp
        WORKING_DIRECTORY ${GENERATOR_DIR}
        OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/glsldebug.def
    )
endif()
