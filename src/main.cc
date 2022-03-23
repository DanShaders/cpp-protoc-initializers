#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/cpp/cpp_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>

#include <vector>
using namespace google::protobuf;

class Generator : public compiler::CodeGenerator {
private:
	template <typename T>
	string convert_scoped(const T *message) const {
		if (message->containing_type()) {
			return convert_scoped(message->containing_type()) + "_" + message->name();
		} else {
			string s = "";
			for (char c : message->full_name()) {
				if (c == '.') {
					s += "::";
				} else {
					s += c;
				}
			}
			return s;
		}
	}

	template <typename T>
	string convert_unscoped(const T *message) const {
		std::vector<string> names;
		while (message) {
			names.push_back(message->name());
			message = message->containing_type();
		}
		reverse(names.begin(), names.end());
		string s;
		for (auto c : names) {
			s += c + "_";
		}
		s.pop_back();
		return s;
	}

	string get_type_name(const FieldDescriptor *field) const {
		using namespace std::literals;

		auto etype = field->cpp_type();

		string type = "::PROTOBUF_NAMESPACE_ID::" + std::string(field->cpp_type_name());

		if (etype == FieldDescriptor::CPPTYPE_ENUM) {
			type = convert_scoped(field->enum_type());
		} else if (etype == FieldDescriptor::CPPTYPE_MESSAGE) {
			if (field->is_map()) {
				type = "std::map<"s + get_type_name(field->message_type()->map_key()) + ", " +
					   get_type_name(field->message_type()->map_value()) + ">";
			} else {
				type = convert_scoped(field->message_type());
			}
		} else if (etype == FieldDescriptor::CPPTYPE_BOOL) {
			type = "bool";
		}
		if (!field->is_map() && field->is_repeated()) {
			type = "std::vector<" + type + ">";
		}
		return type;
	}

	bool GenerateFor(const Descriptor *message, const FileDescriptor *file,
					 compiler::GeneratorContext *context) const {
		auto class_scope_inserter = context->OpenForInsert(
			compiler::StripProto(file->name()) + ".pb.h", "class_scope:" + message->full_name());
		auto printer = io::Printer(class_scope_inserter, '$');

		printer.Print("const static std::string TYPE_NAME;\n");
		printer.Print("struct initializable_type {\n");
		printer.Indent();
		for (int i = 0; i < message->field_count(); ++i) {
			auto field = message->field(i);
			if (field->containing_oneof()) {
				continue;
			}

			string type = get_type_name(field);
			printer.Print("$type$ $name$;\n", "type", type.c_str(), "name", field->name().c_str());
		}
		printer.Outdent();
		printer.Print("};\n\n$constructor$(const initializable_type &t) : $constructor$() {\n",
					  "constructor", convert_unscoped(message));
		printer.Indent();
		for (int i = 0; i < message->field_count(); ++i) {
			auto field = message->field(i);
			if (field->containing_oneof()) {
				continue;
			}

			auto lname = field->lowercase_name().c_str(), name = field->name().c_str();
			if (field->is_map()) {
				printer.Print(
					"for (auto &map = (*mutable_$lname$()); const auto &[key, value] : t.$name$) "
					"{\n",
					"lname", lname, "name", name);
				printer.Indent();
				printer.Print("map[key] = value;\n", "lname", name);
				printer.Outdent();
				printer.Print("}\n");
			} else if (field->is_repeated()) {
				printer.Print("for (std::size_t i = 0; i < t.$name$.size(); ++i) {\n", "name",
							  name);
				printer.Indent();
				printer.Print("*add_$lname$() = t.$name$[i];\n", "lname", lname, "name", name);
				printer.Outdent();
				printer.Print("}\n");
			} else {
				printer.Print("set_$lname$(t.$name$);\n", "lname", lname, "name", name);
			}
		}
		printer.Outdent();
		printer.Print("}\n");

		for (int i = 0; i < message->nested_type_count(); ++i) {
			auto type = message->nested_type(i);
			if (!type->map_key()) {
				GenerateFor(message->nested_type(i), file, context);
			}
		}

		{
			auto namespace_scope_inserter = context->OpenForInsert(
				compiler::StripProto(file->name()) + ".pb.h", "namespace_scope");
			auto p = io::Printer(namespace_scope_inserter, '$');

			p.Print("inline const std::string $1$::TYPE_NAME = \"$2$\";\n", "1",
					convert_scoped(message), "2", message->full_name());
		}
		{
			auto includes_inserter =
				context->OpenForInsert(compiler::StripProto(file->name()) + ".pb.h", "includes");
			io::Printer(includes_inserter, '$').Print("#include <vector>\n");
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