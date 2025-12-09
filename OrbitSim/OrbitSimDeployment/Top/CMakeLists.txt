register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/topology.fpp"

    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/OrbitSimDeploymentTopology.cpp"
    DEPENDS
        Fw_Logger
)
