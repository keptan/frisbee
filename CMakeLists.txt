cmake_minimum_required (VERSION 3.5)

project(frisbee)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(cureMaid/lib/transactional)

add_executable(frisbee
	src/main.cpp
	src/hashDB.cpp
	src/filetypes.cpp
	src/fileMd5.cpp
	src/image.cpp
) 

target_include_directories(frisbee
	PRIVATE
		src
)

target_link_libraries(frisbee
	PRIVATE
		transaction
		ssl
		crypto
		stdc++fs
)

target_compile_features(frisbee PUBLIC cxx_std_17)
