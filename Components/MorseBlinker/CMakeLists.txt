register_fprime_library(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/MorseBlinker.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/MorseBlinker.cpp"
)

find_library(GPIOD_LIB gpiod REQUIRED)
target_link_libraries(${FPRIME_CURRENT_MODULE} PRIVATE ${GPIOD_LIB})
