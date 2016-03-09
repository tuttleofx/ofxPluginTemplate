cmake_minimum_required(VERSION 2.8) 

project(plugin)

# Add openfx subdirectory
add_subdirectory("${PROJECT_SOURCE_DIR}/openfx")

# Build example plugin
create_ofx_plugin("plugin")
