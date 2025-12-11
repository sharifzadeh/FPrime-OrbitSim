#### 
# F Prime CMakeLists.txt:
####

register_fprime_library(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/ImuDriver.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/ImuStatusPort.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/ImuDriver.cpp"
    # DEPENDS
    #   MyPackage_MyOtherModule
)
