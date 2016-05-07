cmake_minimum_required(VERSION 2.8) 

project(PLUGIN_NAMEPlugin)

# Check that submodule have been initialized and updated
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/openfx/include)
  message(FATAL_ERROR
    "\n submodule(s) are missing, please update your repository:\n"
    "  > git submodule update -i\n")
endif()

# Add openfx subdirectory
add_subdirectory("${PROJECT_SOURCE_DIR}/openfx")

# Build PLUGIN_NAME plugin
create_ofx_plugin(PLUGIN_NAMEPlugin src/)

