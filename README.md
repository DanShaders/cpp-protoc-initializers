# cpp-protoc-initializers

**WORK IN PROGRESS**

This plugin allows using aggregate initialization on Protocol Buffer generated classes.

Invocation: `protoc --plugin=protoc-gen-cpp_initializers=<path_to_plugin> --cpp_initializers_out=. PROTO_FILES`.

Example:
```protobuf
syntax = "proto3";

package foo;

message Message {
	int32 field;
}
```

```cpp
template <typename T>
void foo(const typename T::initializable_type &message_) {
	T message(message_);
}

foo<package::Message>({.field = 2});
```
