pack_native_module(omium)

if(APPLE)
    pack_binary_file("Chromium Embedded Framework")
    pack_binary_dir("Resources")
else()
    if(WIN32)
        pack_library(d3dcompiler_43)
        pack_library(d3dcompiler_47)
    endif()

    pack_library(libcef)
    pack_library(libEGL)
    pack_library(libGLESv2)

    pack_binary_file(cef.pak)
    pack_binary_file(cef_100_percent.pak)
    pack_binary_file(cef_200_percent.pak)
    pack_binary_file(cef_extensions.pak)
    pack_binary_file(devtools_resources.pak)
    pack_binary_file(icudtl.dat)
    pack_binary_dir(locales)
endif()

