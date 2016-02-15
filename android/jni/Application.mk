APP_PLATFORM := android-8
APP_ABI := armeabi
#APP_STL := stlport_static
APP_STL := gnustl_static
APP_CPPFLAGS :=-fexceptions
APP_CPPFLAGS +=-std=c++11
APP_CPPFLAGS +=-frtti
#APP_OPTIM := release