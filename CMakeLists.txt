# FIXME: This surely can be lowered.
cmake_minimum_required(VERSION 3.27)

project(cpp-protoc-initializers VERSION 0.0.0)

# FIXME: At least on Arch Linux, /usr/lib/cmake is not in a default search path. Figure out whose
#        responsibility is to add it.
find_package(protobuf/protobuf REQUIRED CONFIG PATHS /usr/lib/cmake)

add_executable(cpp-protoc-initializers src/main.cc)

target_link_libraries(
	cpp-protoc-initializers PRIVATE
	protobuf::libprotobuf
	protobuf::libprotoc
)

set_target_properties(
	cpp-protoc-initializers
	PROPERTIES OUTPUT_NAME protoc-gen-cpp_initializers
)
