pack_native_module(omium)
if(WIN32)
    pack_shared_lib(d3dcompiler_43)
    pack_shared_lib(d3dcompiler_47)
endif()

pack_shared_lib(libcef)
pack_shared_lib(libEGL)
pack_shared_lib(libGLESv2)

pack_binary_file(cef.pak)
pack_binary_file(cef_100_percent.pak)
pack_binary_file(cef_200_percent.pak)
pack_binary_file(cef_extensions.pak)
pack_binary_file(devtools_resources.pak)
pack_binary_file(icudtl.dat)
pack_binary_dir(locales)


