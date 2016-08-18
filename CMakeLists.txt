module_version(1.0)

if(OMEGA_OS_WIN)
    set(CEF_BUILD_FILE cef_binary_3.2704.1432.g60b3718_windows64_minimal)
elseif(OMEGA_OS_OSX)
    set(CEF_BUILD_FILE cef_binary_3.2704.1432.g60b3718_macosx64_minimal)
endif()

# Download and add the CEF3 external project
if(NOT EXISTS ${CMAKE_BINARY_DIR}/3rdparty/cef3)
    message("Downloading CEF3 binaries...")
    set(CEF_BUILD_FILE_DEST ${CMAKE_BINARY_DIR}/3rdparty/cef3/${CEF_BUILD_FILE}.tar.bz2)
    file(DOWNLOAD 
        http://opensource.spotify.com/cefbuilds/${CEF_BUILD_FILE}.tar.bz2 
        ${CEF_BUILD_FILE_DEST} SHOW_PROGRESS)
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${CEF_BUILD_FILE_DEST} WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/3rdparty/cef3)
endif()

set(CEF_ROOT ${CMAKE_BINARY_DIR}/3rdparty/cef3/${CEF_BUILD_FILE})
set(CMAKE_MODULE_PATH ${CEF_ROOT}/cmake)

find_package(CEF REQUIRED)

# Add the libcef_dll_wrapper project
add_subdirectory(${CEF_ROOT}/libcef_dll ${CMAKE_BINARY_DIR}/3rdparty/cef3/build)
set_target_properties(libcef_dll_wrapper PROPERTIES FOLDER 3rdparty)

# Logical target used to link the libcef library.
ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_RELEASE}" "${CEF_LIB_RELEASE}")

if(OMEGA_OS_WIN)
    set(CEF_DLL_WRAPPER_LIB 
        debug ${CMAKE_BINARY_DIR}/lib/debug/libcef_dll_wrapper.lib 
        optimized ${CMAKE_BINARY_DIR}/lib/release/libcef_dll_wrapper.lib )
elseif(OMEGA_OS_OSX)
    set(CEF_DLL_WRAPPER_LIB 
        ${CMAKE_BINARY_DIR}/lib/libcef_dll_wrapper.a) 
endif()

include_directories(${CEF_ROOT} ${CEF_ROOT}/include)
add_library(omium MODULE  omium.cpp)
if(OMEGA_OS_WIN)
    add_executable(omium_process WIN32 omium_process.cpp)
else()
    add_executable(omium_process omium_process.cpp)
endif()
set_target_properties(omium_process PROPERTIES FOLDER modules)

if(OMEGA_OS_WIN)
	target_compile_options(omium PUBLIC $<$<CONFIG:Debug>:/MTd>) #${CEF_CXX_COMPILER_FLAGS_DEBUG}>)
	target_compile_options(omium PUBLIC $<$<CONFIG:Release>:/MT>)# ${CEF_CXX_COMPILER_FLAGS_RELEASE}>)
else()
	target_compile_options(omium PUBLIC $<$<CONFIG:Debug>:${CEF_CXX_COMPILER_FLAGS_DEBUG}>)
	target_compile_options(omium PUBLIC $<$<CONFIG:Release>:${CEF_CXX_COMPILER_FLAGS_RELEASE}>)
endif()

message("${CEF_DLL_WRAPPER_LIB} ${CEF_STANDARD_LIBS}")
target_link_libraries(omium omega libcef_lib ${CEF_DLL_WRAPPER_LIB} ${CEF_STANDARD_LIBS})

if(OMEGA_OS_WIN)
	target_compile_options(omium_process PUBLIC $<$<CONFIG:Debug>:/MTd>) #${CEF_CXX_COMPILER_FLAGS_DEBUG}>)
	target_compile_options(omium_process PUBLIC $<$<CONFIG:Release>:/MT>)# ${CEF_CXX_COMPILER_FLAGS_RELEASE}>)
else()
	target_compile_options(omium_process PUBLIC $<$<CONFIG:Debug>:${CEF_CXX_COMPILER_FLAGS_DEBUG}>)
	target_compile_options(omium_process PUBLIC $<$<CONFIG:Release>:${CEF_CXX_COMPILER_FLAGS_RELEASE}>)
endif()
target_link_libraries(omium_process libcef_lib ${CEF_DLL_WRAPPER_LIB} ${CEF_STANDARD_LIBS})

declare_native_module(omium)

add_dependencies(omium libcef_dll_wrapper)
add_dependencies(omium_process libcef_dll_wrapper)

if(OS_WINDOWS)
  # Copy binary and resource files to the target output directory.
  COPY_FILES("omium" "${CEF_BINARY_FILES}" "${CEF_ROOT}/Release" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG}")
  COPY_FILES("omium" "${CEF_RESOURCE_FILES}" "${CEF_ROOT}/Resources" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG}")
  COPY_FILES("omium" "${CEF_BINARY_FILES}" "${CEF_ROOT}/Release" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}")
  COPY_FILES("omium" "${CEF_RESOURCE_FILES}" "${CEF_ROOT}/Resources" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}")
else()
  # Copy binary and resource files to the target output directory.
  COPY_FILES("omium" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
  COPY_FILES("omium" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
endif()
