# PSn00bSDK example CMake script
# (C) 2021 spicyjpeg - MPL licensed

cmake_minimum_required(VERSION 3.21)

project(
	spustream
	LANGUAGES    C ASM
	VERSION      1.0.0
	DESCRIPTION  "Sandbox_PSX by alimovlex"
	HOMEPAGE_URL "http://recompile.me"
)

include_directories(inc)

file(GLOB _sources main.c ./src/*.c)

psn00bsdk_add_executable(spustream GPREL ${_sources})
psn00bsdk_add_cd_image(spustream_iso spustream iso.xml DEPENDS spustream)

psn00bsdk_target_incbin(spustream PRIVATE stream_data stream.vag)

install(FILES ${PROJECT_BINARY_DIR}/spustream.exe TYPE BIN)
