file(GLOB_RECURSE SRC_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/*.cpp"    
    "${CMAKE_CURRENT_SOURCE_DIR}/modules/*.c"    
)

idf_component_register(
    SRCS ${SRC_FILES}
    INCLUDE_DIRS 
        "."
        "modules"        
)                