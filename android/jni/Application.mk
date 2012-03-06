#Can also set this to other values such as APP_ABI := armeabi armeabi-v7a x86
APP_ABI := armeabi
APP_STL := gnustl_static
# Need rtti for shared_ptr support to work on NDK r7b
APP_CPPFLAGS := -fexceptions -frtti
