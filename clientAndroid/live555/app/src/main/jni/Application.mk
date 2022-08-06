APP_PLATFORM := android-21
#下面这个配置非常重要，不然由于c＋＋库的问题，各种标准库链接符号问题。。。
#https://developer.android.google.cn/ndk/guides/cpp-support
APP_STL := c++_shared
APP_CPPFLAGS := -frtti  -fexceptions
#APP_STL := c++_static

APP_ABI :=arm64-v8a
#APP_ABI +=armeabi
#APP_ABI +=armeabi-v7a
APP_CPPFLAGS += -Wno-error=format-security -std=c++17