#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/cpp/cpp_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>
using namespace google::protobuf;

class Generator : public compiler::CodeGenerator {
private:
	bool GenerateFor(const Descriptor *message, const FileDescriptor *file,
					 compiler::GeneratorContext *context) const {
		auto class_scope_inserter = context->OpenForInsert(
			compiler::StripProto(file->name()) + ".pb.h", "class_scope:" + message->full_name());
		auto printer = io::Printer(class_scope_inserter, '$');

		printer.Print("struct initializable_type {\n");
		printer.Indent();
		for (int i = 0; i < message->field_count(); ++i) {
			auto field = message->field(i);
			printer.Print("::PROTOBUF_NAMESPACE_ID::$type$ $name$;\n", "type",
						  field->cpp_type_name(), "name", field->name().c_str());
		}
		printer.Outdent();
		printer.Print("};\n\n$constructor$(const initializable_type &t) {\n", "constructor",
					  message->name());
		printer.Indent();
		for (int i = 0; i < message->field_count(); ++i) {
			auto field = message->field(i);
			printer.Print("set_$lname$(t.$name$);\n", "lname", field->lowercase_name().c_str(),
						  "name", field->name().c_str());
		}
		printer.Outdent();
		printer.Print("}\n");

		for (int i = 0; i < message->nested_type_count(); ++i) {
			GenerateFor(message->nested_type(i), file, context);
		}
		return true;
	}

	compiler::cpp::CppGenerator cppGenerator;

public:
	bool Generate(const FileDescriptor *file, const string &parameter,
				  compiler::GeneratorContext *context, string *error) const {
		if (!cppGenerator.Generate(file, parameter, context, error)) {
			return false;
		}
		for (int i = 0; i < file->message_type_count(); ++i) {
			GenerateFor(file->message_type(i), file, context);
		}
		return true;
	}
};

int main(int argc, char **argv) {
	Generator generator;
	return compiler::PluginMain(argc, argv, &generator);
}