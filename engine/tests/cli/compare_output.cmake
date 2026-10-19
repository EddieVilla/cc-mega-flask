# Runs a CLI on a script and checks its output matches an expected file.
#
#   cmake -DCLI=<program> -DINPUT=<script> -DEXPECTED=<file> -P compare_output.cmake

execute_process(
    COMMAND "${CLI}" "${INPUT}"
    OUTPUT_VARIABLE actual
    ERROR_VARIABLE errors
    RESULT_VARIABLE exit_code)
if(NOT exit_code EQUAL 0)
    message(FATAL_ERROR "${CLI} exited with ${exit_code}\n${errors}")
endif()

file(READ "${EXPECTED}" expected)
# Ignore Windows line endings on either side.
string(REPLACE "\r\n" "\n" actual "${actual}")
string(REPLACE "\r\n" "\n" expected "${expected}")
if(NOT actual STREQUAL expected)
    message(FATAL_ERROR
        "Output differs from ${EXPECTED}\n"
        "--- expected ---\n${expected}"
        "--- actual ---\n${actual}")
endif()
