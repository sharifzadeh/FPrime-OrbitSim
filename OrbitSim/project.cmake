# This CMake file is intended to register project-wide objects.
# This allows for reuse between deployments, or other projects.

# Explicitly add each component directory
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Components/MorseBlinker")

# Deployment
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/OrbitSimDeployment")
