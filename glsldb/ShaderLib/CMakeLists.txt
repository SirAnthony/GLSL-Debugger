
find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)

file(GLOB SRC_CPP *.cpp data/*.cpp models/*.cpp docks/*.cpp dialogs/*.cpp watch/*.cpp)
file(GLOB MOC_HEADERS *.h data/*.h models/*.h docks/*.h dialogs/*.h watch/*.h)
file(GLOB UI_FILES ui/*.ui)
qt4_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS})
qt4_wrap_ui(UI_OUTFILES ${UI_FILES})

include(${QT_USE_FILE})

include_directories(
	"${CMAKE_CURRENT_BINARY_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/glsldb"
	"${PROJECT_SOURCE_DIR}/glsldb/DebugLib"
	${QT_INCLUDE_DIR}
	${GLSL_INCLUDES}
)

set(SRC ${SRC_CPP} ${MOC_OUTFILES} ${UI_OUTFILES} ${RES_OUTFILES})
add_library(shadergui STATIC ${SRC})
target_link_libraries(shadergui ${GLSL_LIBRARIES})

#if(TESTS)
    add_subdirectory(tests)
#endif()