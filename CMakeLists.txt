module_version(1.0)

add_library(omium MODULE 
    omium.cpp)
    
target_link_libraries(omium omega)

declare_native_module(omium)