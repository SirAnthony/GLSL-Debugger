
find_package(Qt4 REQUIRED COMPONENTS QtCore QtGui)

file(GLOB SRC_CPP *.cpp data/*.cpp)
file(GLOB MOC_HEADERS data/*.hdata/*.h *.h)
file(GLOB UI_FILES ui/*.ui)
qt4_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS})
qt4_wrap_ui(UI_OUTFILES ${UI_FILES})

include(${QT_USE_FILE})

if(USE_MESA)
    set(GLSL_INCLUDES "${PROJECT_SOURCE_DIR}/mesa-glsl/include")
    set(GLSL_LIBRARIES glsl_mesa_interface)
else()
    set(GLSL_INCLUDES
	"${PROJECT_SOURCE_DIR}/GLSLCompiler/glslang/Public"
	"${PROJECT_SOURCE_DIR}/GLSLCompiler/glslang/Include"
    )
    set(GLSL_LIBRARIES glslang)
endif()

include_directories(
	"${CMAKE_CURRENT_BINARY_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/glsldb"
	"${PROJECT_SOURCE_DIR}/glsldb/DebugLib"
	${QT_INCLUDE_DIR}
	${GLSL_INCLUDES}
)

set(SRC ${SRC_CPP} ${MOC_OUTFILES} ${UI_OUTFILES} ${RES_OUTFILES})
add_library(shaderlib STATIC ${SRC})
