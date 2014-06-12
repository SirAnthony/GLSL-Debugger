
find_package(Qt4 COMPONENTS QtTest REQUIRED)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

set(TESTS_SRC
	runner.cpp
)

add_executable(shadergui_tests ${TESTS_SRC})
target_link_libraries(shadergui_tests utils ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY})
add_test(NAME "TestShaderLib" COMMAND "${BINARY_DIR}/shadergui_tests" "${CMAKE_CURRENT_SOURCE_DIR}/")
