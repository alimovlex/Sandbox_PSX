# PSn00bSDK example CMake script
# (C) 2021 spicyjpeg - MPL licensed

cmake_minimum_required(VERSION 3.20)

project(
	Sandbox
	LANGUAGES    C CXX ASM
	VERSION      1.0.0
	DESCRIPTION  "built with PSn00bSDK"
	HOMEPAGE_URL "http://recompile.me"
)
include_directories(inc)
file(GLOB _sources main.c ./src/*.c)
psn00bsdk_add_executable(bandwidth STATIC ${_sources})
psn00bsdk_add_cd_image(iso bandwidth iso.xml DEPENDS bandwidth)
psn00bsdk_target_incbin(bandwidth PRIVATE stream_data stream.vag)

install(FILES ${PROJECT_BINARY_DIR}/bandwidth.exe TYPE BIN)


