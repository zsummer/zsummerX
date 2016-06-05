cmake_minimum_required(VERSION 2.6)
project(genProto)


add_definitions(-Wall  -O0 -g -ggdb  -DNDEBUG -std=c++11 -D_GLIBCXX_USE_NANOSLEEP )

set(EXECUTABLE_OUTPUT_PATH ${PROTO4Z_GEN_CODE_PATH})

aux_source_directory(./src source)
aux_source_directory(./depends depends)
aux_source_directory(./depends/md5 md5)

if (APPLE)
	add_executable(genProtoMac ${source} ${depends} ${md5})
	target_link_libraries(genProtoMac pthread)
else()
	add_executable(genProto  ${source} ${depends} ${md5})
	target_link_libraries(genProto pthread rt)
endif()
