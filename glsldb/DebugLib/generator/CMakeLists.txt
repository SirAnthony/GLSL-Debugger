
find_package(Perl REQUIRED)


set(GENERATOR_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(MAKE_DIRECTORY ${GENERATOR_OUTPUT_DIR})

# Read gl and build api definition
message(STATUS "Reading gl headers")
execute_process(COMMAND ${PERL_EXECUTABLE}
    WORKING_DIRECTORY ${GENERATOR_DIR}
    INPUT_FILE glreader.pl
    OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/glheaders.pm
)


# Generate enumerants
message(STATUS "Generate enumerants")
execute_process(
    COMMAND ${PERL_EXECUTABLE} Enumerants.pl -p ${GENERATOR_OUTPUT_DIR}
    WORKING_DIRECTORY ${GENERATOR_DIR}
    OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/glenumerants.h
)

#~ if(GLSLDB_LINUX OR GLSLDB_OSX)
    #~ execute_process(COMMAND ${PERL_EXECUTABLE} Enumerants.pl -m glx
        #~ WORKING_DIRECTORY ${GENERATOR_DIR}
        #~ OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/glxenumerants.h
    #~ )
#~ endif()
#~ if(GLSLDB_WIN)
    #~ execute_process(COMMAND ${PERL_EXECUTABLE} Enumerants.pl -m wgl
        #~ WORKING_DIRECTORY ${GENERATOR_DIR}
        #~ OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/wglenumerants.h
    #~ )
#~ endif()


# Generate generator for list of allowed functions between begin/end
message(STATUS "Generate allowed function list generator.")
execute_process(COMMAND ${PERL_EXECUTABLE}
    WORKING_DIRECTORY ${GENERATOR_DIR}
    INPUT_FILE BeginEndFunctionTest.pl
    OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/beginEndFunctionTest.c
)


# Generate debug library
message(STATUS "Generate debug library")
execute_process(COMMAND ${PERL_EXECUTABLE}
    WORKING_DIRECTORY ${GENERATOR_DIR}
    INPUT_FILE GetProcAddressHook.pl
    OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/getProcAddressHook.inc
)
execute_process(COMMAND ${PERL_EXECUTABLE}
    WORKING_DIRECTORY ${GENERATOR_DIR}
    INPUT_FILE FunctionPointerTypes.pl
    OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/functionPointerTypes.inc
)
execute_process(COMMAND ${PERL_EXECUTABLE}
    WORKING_DIRECTORY ${GENERATOR_DIR}
    INPUT_FILE ReplayFunc.pl
    OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/replayFunction.c
)
execute_process(COMMAND ${PERL_EXECUTABLE}
    WORKING_DIRECTORY ${GENERATOR_DIR}
    INPUT_FILE FunctionList.pl
    OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/functionList.c
)
execute_process(COMMAND ${PERL_EXECUTABLE}
    WORKING_DIRECTORY ${GENERATOR_DIR}
    INPUT_FILE mergeAllowedInBeginEndList.pl
    OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/functionsAllowedInBeginEnd.pm
)
execute_process(COMMAND ${PERL_EXECUTABLE} FunctionHooks.pl -p ${GENERATOR_OUTPUT_DIR}
    WORKING_DIRECTORY ${GENERATOR_DIR}
    OUTPUT_FILE ${GENERATOR_OUTPUT_DIR}/functionHooks.inc
)




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
