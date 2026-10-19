include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE)
# Keep GoogleTest from overriding our compiler/linker settings on Windows.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(me_tests
    test_instrument.cpp
    test_ladder.cpp
    test_order_book.cpp
    test_price.cpp
    test_price_level.cpp
    test_sanity.cpp
    test_types.cpp
    test_validation.cpp)
target_link_libraries(me_tests PRIVATE me_core GTest::gmock GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(me_tests)

# End-to-end tests: run me_cli on each script in cli/ and compare its output
# with the matching .expected file.
foreach(script book_basics)
    add_test(NAME cli.${script}
        COMMAND ${CMAKE_COMMAND}
            -DCLI=$<TARGET_FILE:me_cli>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/cli/${script}.in
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/cli/${script}.expected
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cli/compare_output.cmake)
endforeach()
