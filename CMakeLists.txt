cmake_minimum_required(VERSION 3.20)

project(
    asm_studio
    VERSION 0.1.0
    DESCRIPTION "C++ compile-time-aware assembler studio."
    LANGUAGES CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ---------------------------------------------------------------------------
# C++ standard selection — supports 17, 20, or 23.
# ---------------------------------------------------------------------------
get_property(ASM_CXX_STANDARD_CACHE_TYPE CACHE ASM_CXX_STANDARD PROPERTY TYPE)
if(ASM_CXX_STANDARD_CACHE_TYPE STREQUAL "BOOL")
    set(ASM_CXX_STANDARD_MIGRATED_VALUE "${ASM_CXX_STANDARD}")
    if(NOT ASM_CXX_STANDARD_MIGRATED_VALUE MATCHES "^(17|20|23)$")
        set(ASM_CXX_STANDARD_MIGRATED_VALUE "23")
    endif()
    unset(ASM_CXX_STANDARD CACHE)
    set(ASM_CXX_STANDARD "${ASM_CXX_STANDARD_MIGRATED_VALUE}" CACHE STRING "Target C++ standard (17, 20, 23)")
endif()
set(ASM_CXX_STANDARD "23" CACHE STRING "Target C++ standard (17, 20, 23)")
if(NOT ASM_CXX_STANDARD MATCHES "^(17|20|23)$")
    message(FATAL_ERROR "ASM_CXX_STANDARD must be 17, 20, or 23. Got: ${ASM_CXX_STANDARD}")
endif()
set(CMAKE_CXX_STANDARD ${ASM_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTING "Enable testing for this project." ${PROJECT_IS_TOP_LEVEL})
option(ASM_BUILD_TESTS "Build the asm_studio test suite." ${PROJECT_IS_TOP_LEVEL})
option(ASM_ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan in debug builds." ON)

set(ASM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(ASM_SRC_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(ASM_TEST_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/tests")

# Compiler warning flags (applied to our targets only).
set(ASM_WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wnon-virtual-dtor
)

# Sanitizer flags for debug builds.
set(ASM_SANITIZER_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer)

# ---------------------------------------------------------------------------
# asm_studio_lib — static library with all logic (no main).
# Tests and the executable both link against this.
# ---------------------------------------------------------------------------
add_library(asm_studio_lib STATIC
    # support — utilities and diagnostics
    ${ASM_SRC_DIR}/support/Diagnostic.cpp
    # dsl — C++ builder API
    ${ASM_SRC_DIR}/dsl/Program.cpp
    ${ASM_SRC_DIR}/dsl/Function.cpp
    ${ASM_SRC_DIR}/dsl/Variable.cpp
    # ir — types and lowering
    ${ASM_SRC_DIR}/ir/Lowering.cpp
    # transforms — optimizer and passes
    ${ASM_SRC_DIR}/transforms/Optimizer.cpp
    ${ASM_SRC_DIR}/transforms/passes/ConstantFoldingPass.cpp
    ${ASM_SRC_DIR}/transforms/passes/DeadCodeEliminationPass.cpp
    ${ASM_SRC_DIR}/transforms/passes/UnreachableBlockEliminationPass.cpp
    ${ASM_SRC_DIR}/transforms/passes/JumpSimplificationPass.cpp
    # codegen — backend emitters
    ${ASM_SRC_DIR}/codegen/PseudoEmitter.cpp
    ${ASM_SRC_DIR}/codegen/Arm64Emitter.cpp
    # sim — step-based IR interpreter
    ${ASM_SRC_DIR}/sim/Simulator.cpp
    # analysis — IR analysis and graph tools
    ${ASM_SRC_DIR}/analysis/Explain.cpp
    ${ASM_SRC_DIR}/analysis/CfgDot.cpp
    # driver — CLI presentation layer
    ${ASM_SRC_DIR}/driver/ConsolePresenter.cpp
)
add_library(asm::studio ALIAS asm_studio_lib)

target_include_directories(asm_studio_lib PUBLIC
    $<BUILD_INTERFACE:${ASM_INCLUDE_DIR}>
)
target_compile_features(asm_studio_lib PUBLIC cxx_std_${ASM_CXX_STANDARD})
target_compile_options(asm_studio_lib PRIVATE ${ASM_WARNING_FLAGS})
set_target_properties(
    asm_studio_lib
    PROPERTIES
        CXX_EXTENSIONS OFF
        FOLDER "asm_studio"
)

if(ASM_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(asm_studio_lib PUBLIC ${ASM_SANITIZER_FLAGS})
    target_link_options(asm_studio_lib PUBLIC ${ASM_SANITIZER_FLAGS})
endif()

# ---------------------------------------------------------------------------
# asm_studio — main executable.
# ---------------------------------------------------------------------------
add_executable(asm_studio ${ASM_SRC_DIR}/main.cpp)
target_link_libraries(asm_studio PRIVATE asm::studio)
target_compile_options(asm_studio PRIVATE ${ASM_WARNING_FLAGS})
set_target_properties(
    asm_studio
    PROPERTIES
        CXX_EXTENSIONS OFF
        FOLDER "asm_studio"
)

# ---------------------------------------------------------------------------
# Tests.
# ---------------------------------------------------------------------------
if(ASM_BUILD_TESTS AND BUILD_TESTING)
    enable_testing()
    include(GoogleTest)

    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        message(STATUS "GTest not found — fetching GoogleTest v1.15.2.")
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    # Helper: create a focused test binary.
    set(ASM_TEST_TARGETS)
    function(asm_add_test_executable target_name source_path)
        add_executable(${target_name} ${source_path})
        target_link_libraries(${target_name} PRIVATE asm::studio GTest::gtest_main)
        target_compile_features(${target_name} PRIVATE cxx_std_${ASM_CXX_STANDARD})
        target_compile_options(${target_name} PRIVATE ${ASM_WARNING_FLAGS})
        set_target_properties(
            ${target_name}
            PROPERTIES
                CXX_EXTENSIONS OFF
                FOLDER "tests"
        )
        gtest_discover_tests(
            ${target_name}
            TEST_PREFIX "asm."
            DISCOVERY_TIMEOUT 60
        )
        list(APPEND ASM_TEST_TARGETS ${target_name})
        set(ASM_TEST_TARGETS "${ASM_TEST_TARGETS}" PARENT_SCOPE)
    endfunction()

    asm_add_test_executable(asm_support_tests   ${ASM_TEST_DIR}/support/DiagnosticTest.cpp)
    asm_add_test_executable(asm_types_tests     ${ASM_TEST_DIR}/support/TypesTest.cpp)
    asm_add_test_executable(asm_dsl_tests       ${ASM_TEST_DIR}/dsl/ProgramApiTest.cpp)
    asm_add_test_executable(asm_condition_tests ${ASM_TEST_DIR}/dsl/ConditionTest.cpp)
    asm_add_test_executable(asm_expr_stmt_tests ${ASM_TEST_DIR}/dsl/ExprStmtTest.cpp)
    asm_add_test_executable(asm_ir_tests        ${ASM_TEST_DIR}/ir/IRTest.cpp)
    asm_add_test_executable(asm_parse_tests     ${ASM_TEST_DIR}/parse/LexerTest.cpp)
    asm_add_test_executable(asm_parser_tests    ${ASM_TEST_DIR}/parse/ParserTest.cpp)
    asm_add_test_executable(asm_sim_tests       ${ASM_TEST_DIR}/sim/SimulatorTest.cpp)
    asm_add_test_executable(asm_opt_tests       ${ASM_TEST_DIR}/transforms/OptimizerTest.cpp)
    asm_add_test_executable(asm_analysis_tests  ${ASM_TEST_DIR}/analysis/ToolingTest.cpp)

    # Aggregate binary: all test sources compiled together.
    add_executable(asm_all_tests
        ${ASM_TEST_DIR}/support/DiagnosticTest.cpp
        ${ASM_TEST_DIR}/support/TypesTest.cpp
        ${ASM_TEST_DIR}/dsl/ProgramApiTest.cpp
        ${ASM_TEST_DIR}/dsl/ConditionTest.cpp
        ${ASM_TEST_DIR}/dsl/ExprStmtTest.cpp
        ${ASM_TEST_DIR}/ir/IRTest.cpp
        ${ASM_TEST_DIR}/parse/LexerTest.cpp
        ${ASM_TEST_DIR}/parse/ParserTest.cpp
        ${ASM_TEST_DIR}/sim/SimulatorTest.cpp
        ${ASM_TEST_DIR}/transforms/OptimizerTest.cpp
        ${ASM_TEST_DIR}/analysis/ToolingTest.cpp
    )
    target_link_libraries(asm_all_tests PRIVATE asm::studio GTest::gtest_main)
    target_compile_features(asm_all_tests PRIVATE cxx_std_${ASM_CXX_STANDARD})
    target_compile_options(asm_all_tests PRIVATE ${ASM_WARNING_FLAGS})
    set_target_properties(
        asm_all_tests
        PROPERTIES
            CXX_EXTENSIONS OFF
            FOLDER "tests"
    )

    add_custom_target(asm_check DEPENDS ${ASM_TEST_TARGETS})
    set_target_properties(asm_check PROPERTIES FOLDER "tests")
endif()
