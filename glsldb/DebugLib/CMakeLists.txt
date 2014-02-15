find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

if(GLSLDB_LINUX OR GLSLDB_OSX)
    find_package(DL REQUIRED)
    find_package(X11 REQUIRED)
    set(DLSYM_SRC libdlsym.c)
endif()
if(GLSLDB_WIN)
    #add_definitions(/DGLSLDEBUGLIB_HOST)
    set(DL_LIBRARIES mhook ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
    include_directories(
        ${PROJECT_SOURCE_DIR}/3rdparty/mhook/mhook-lib
        ${CMAKE_CURRENT_SOURCE_DIR}/../../GLSLCompiler/glslang/OSDependent/Windows
    )
endif()

# Generate debuglib
set(GENERATOR_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
add_subdirectory(generator)

set(ENUMERANTS_SRC glenumerants.c)
set(BEGINENDTEST_SRC ${GENERATOR_OUTPUT_DIR}/beginEndFunctionTest.c)

set(GLSLDEBUG_GEN_SRC
    ${GENERATOR_OUTPUT_DIR}/replayFunction.c
    ${GENERATOR_OUTPUT_DIR}/functionList.c
)

if(GLSLDB_WIN)
    set(GLSLDEBUG_OS_SRC initLib.c)
    set(GLSLDEBUG_GEN_SRC ${GLSLDEBUG_GEN_SRC} ${GENERATOR_OUTPUT_DIR}/glsldebug.def)
    set(CLIENT_SRC debugClient.c)
endif()


set(GLSLDEBUG_SRC
    libglsldebug.c
    streamRecorder.c
    streamRecording.c
    glstate.c
    readback.c
    shader.c
    error.c
    memory.c
    hooks.c
    queries.c
    preExecution.c
    postExecution.c
    ${GLSLDEBUG_OS_SRC}
    ${GLSLDEBUG_GEN_SRC}
)


include_directories(
    ${PROJECT_SOURCE_DIR}/glsldb
    ${PROJECT_SOURCE_DIR}/glsldb/DebugLib
    ${PROJECT_SOURCE_DIR}/glsldb/utils
    ${CMAKE_CURRENT_BINARY_DIR}
    ${X11_INCLUDE_DIR}
)


add_library(glenumerants STATIC ${ENUMERANTS_SRC})

#add_executable(beginEndFunctionTest ${BEGINENDTEST_SRC})
#target_link_libraries(beginEndFunctionTest ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})

add_library(functionList STATIC ${GENERATOR_OUTPUT_DIR}/functionList.c)

add_library(glsldebug SHARED ${GLSLDEBUG_SRC})
target_link_libraries(glsldebug utils glenumerants ${DL_LIBRARIES})

if(GLSLDB_LINUX OR GLSLDB_OSX)
    add_library(dlsym SHARED ${DLSYM_SRC})
    target_link_libraries(dlsym ${DL_LIBRARIES})
endif()

if(GLSLDB_WIN)
    add_executable(debugclient ${CLIENT_SRC})
    target_link_libraries(debugclient glsldebug utils)
    install(TARGETS glsldebug #beginEndFunctionTest
        RUNTIME DESTINATION ${DISTRIBUTION_DIRECTORY} LIBRARY DESTINATION ${DISTRIBUTION_DIRECTORY})
endif()
