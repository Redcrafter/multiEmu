#include "json.h"
using namespace std::string_literals;

Json& JsonBase::operator[](const char* name) { throw std::runtime_error("not allowed"); }
Json& JsonBase::operator[](const std::string& name) { throw std::runtime_error("not allowed"); }
Json& JsonBase::operator[](const size_t pos) { throw std::runtime_error("not allowed"); }

JsonBase::operator std::string() const { throw std::runtime_error("not allowed"); }

JsonBase::operator double() const { throw std::runtime_error("not allowed"); }
JsonBase::operator float() const { throw std::runtime_error("not allowed"); }

JsonBase::operator uint64_t() const { throw std::runtime_error("not allowed"); }
JsonBase::operator uint32_t() const { throw std::runtime_error("not allowed"); }
JsonBase::operator uint16_t() const { throw std::runtime_error("not allowed"); }
JsonBase::operator uint8_t() const { throw std::runtime_error("not allowed"); }

JsonBase::operator int64_t() const { throw std::runtime_error("not allowed"); }
JsonBase::operator int32_t() const { throw std::runtime_error("not allowed"); }
JsonBase::operator int16_t() const { throw std::runtime_error("not allowed"); }
JsonBase::operator int8_t() const { throw std::runtime_error("not allowed"); }

JsonBase::operator bool() const { throw std::runtime_error("not allowed"); }



JsonObject::JsonObject(const std::map<std::string, Json>& elements) : elements(elements) { }

std::map<std::string, Json>::iterator JsonObject::begin() { return elements.begin(); }
std::map<std::string, Json>::iterator JsonObject::end() { return elements.end(); }

void JsonObject::print(std::ostream& stream) const {
	stream << "{";
	bool first = true;
	for(auto& item : elements) {
		if(first) {
			first = false;
		} else {
			stream << ',';
		}
		stream << '"' << item.first << "\":" << item.second;
	}
	stream << "}";
}

Json& JsonObject::operator[](const char* name) { return elements[std::string(name)]; }
Json& JsonObject::operator[](const std::string& name) { return elements[name]; }

JsonObject::operator bool() const { return true; }



JsonArray::JsonArray(const std::vector<Json>& elements) : elements(elements) { }

std::vector<Json>::iterator JsonArray::begin() { return elements.begin(); }
std::vector<Json>::iterator JsonArray::end() { return elements.end(); }

void JsonArray::print(std::ostream& stream) const {
	stream << "[";
	bool first = true;
	for(auto& item : elements) {
		if(first) {
			first = false;
		} else {
			stream << ',';
		}
		stream << item;
	}
	stream << "]";
}

Json& JsonArray::operator[](size_t pos) { return elements[pos]; }

JsonArray::operator bool() const { return true; };



JsonString::JsonString(const std::string& value) : value(value) { }

void JsonString::print(std::ostream& stream) const {
	stream << '"';
	for(char c : value) {
		switch(c) {
			case '"': stream << "\\\""; break;
			case '\\': stream << "\\\\"; break;
			case '\n': stream << "\\n"; break;
			case '\r': stream << "\\r"; break;
			case '\t': stream << "\\t"; break;
			case '\b': stream << "\\b"; break;
			case '\f': stream << "\\f"; break;
			default: stream << c;
		}
	}
	stream << '"';
}

JsonString::operator double() const { return std::stod(value); }
JsonString::operator float() const { return std::stof(value); }

JsonString::operator uint64_t() const { return std::stoull(value); }
JsonString::operator uint32_t() const { return std::stoul(value); }
JsonString::operator uint16_t() const { return std::stoul(value); }
JsonString::operator uint8_t() const { return std::stoul(value); }

JsonString::operator int64_t() const { return std::stoll(value); }
JsonString::operator int32_t() const { return std::stoi(value); }
JsonString::operator int16_t() const { return std::stoi(value); }
JsonString::operator int8_t() const { return std::stoi(value); }

JsonString::operator std::string() const { return value; }



JsonNumber::JsonNumber(double value) : value(value) { }

void JsonNumber::print(std::ostream& stream) const {
	stream << value;
}

JsonNumber::operator double() const { return value; }
JsonNumber::operator float() const { return value; }

JsonNumber::operator uint64_t() const { return value; }
JsonNumber::operator uint32_t() const { return value; }
JsonNumber::operator uint16_t() const { return value; }
JsonNumber::operator uint8_t() const { return value; }

JsonNumber::operator int64_t() const { return value; }
JsonNumber::operator int32_t() const { return value; }
JsonNumber::operator int16_t() const { return value; }
JsonNumber::operator int8_t() const { return value; }

JsonNumber::operator bool() const { return value; }



JsonBool::JsonBool(bool value) : value(value) { }

void JsonBool::print(std::ostream& stream) const { 
	stream << (value ? "true" : "false"); 
}

JsonBool::operator double() const { return value; }
JsonBool::operator float() const { return value; }

JsonBool::operator uint64_t() const { return value; }
JsonBool::operator uint32_t() const { return value; }
JsonBool::operator uint16_t() const { return value; }
JsonBool::operator uint8_t() const { return value; }

JsonBool::operator int64_t() const { return value; }
JsonBool::operator int32_t() const { return value; }
JsonBool::operator int16_t() const { return value; }
JsonBool::operator int8_t() const { return value; }

JsonBool::operator bool() const { return value; }



static bool isNumber(char c) {
	return c >= 48 && c <= 57;
}

struct JsonParser {
	std::istream& stream;
	int currentChar;
	
	int line = 1;
	int row = 1;

	JsonParser(std::istream& str) : stream(str) {
		currentChar = stream.get();
		skipSpace();
	}

	void skipSpace() {
		while(currentChar == ' ' || currentChar == '\n' || currentChar == '\r' || currentChar == '\t') {
			if(currentChar == '\n') {
				line++;
				row = 1;
			} else {
				row++;
			}
			currentChar = stream.get();
		}
	}
	void accept() {
		acceptSingle();
		skipSpace();
	}
	void accept(char c) {
		if(currentChar != c) {
			auto msg = "Unexpected symbol '"s + (char)currentChar + "' at (" + std::to_string(line) + "," + std::to_string(row) + ") expected '" + c + "'";
			throw std::runtime_error(msg.c_str());
		}
		accept();
	}
	void acceptSingle() {
		row++;
		currentChar = stream.get();
	}

	std::shared_ptr<JsonBase> parse() {
		switch(currentChar) {
			case '{': return parseObject();
			case '[': return parseArray();
			case '"': return parseString();
			case 't':
			case 'f': return parseBool();
			case 'n': return parseNull();
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				return parseNumber();
			default: throw std::runtime_error("Unexpected char");
		}
	}
	std::shared_ptr<JsonObject> parseObject() {
		accept('{');

		std::map<std::string, Json> elements;
		while(currentChar != '}') {
			auto key = parseString();
			accept(':');
			auto val = parse();

			if(elements.count(key->value)) {
				throw std::runtime_error("Duplicate key");
			} else {
				elements.insert(std::make_pair(key->value, Json(val)));
			}

			if(currentChar == ',') {
				accept();
			} else {
				break;
			}
		}
		accept('}');
		return std::make_shared<JsonObject>(elements);
	}
	std::shared_ptr<JsonArray> parseArray() {
		accept('[');

		std::vector<Json> elements;
		while(currentChar != ']') {
			elements.emplace_back(parse());
			if(currentChar == ',') {
				accept();
			} else {
				break;
			}
		}
		accept(']');

		return std::make_shared<JsonArray>(elements);
	}
	std::shared_ptr<JsonString> parseString() {
		accept('"');
		std::string str;

		while(currentChar != -1) {
			if(currentChar == '\\') {
				acceptSingle();
				switch(currentChar) {
					case '"': str += '"'; break;
					case '\\': str += '\\'; break;
					case 'n': str += '\n'; break;
					case 'r': str += '\r'; break;
					case 't': str += '\t'; break;
					case 'b': str += '\b'; break;
					case 'f': str += '\f'; break;
					default: throw std::runtime_error("Unknown escape character");
				}
				acceptSingle();
			} else if(currentChar == '"') {
				break;
			} else {
				str += (char)currentChar;
				acceptSingle();
			}
		}
		accept('"');
		return std::make_shared<JsonString>(str);
	}
	std::shared_ptr<JsonNumber> parseNumber() {
		double num = 0;
		bool neg = false;

		if(currentChar == '-') {
			neg = true;
			acceptSingle();
		}

		if(isNumber(currentChar)) {
			while(isNumber(currentChar)) {
				num *= 10;
				num += currentChar - 48;
				acceptSingle();
			}

			if(currentChar == '.') {
				acceptSingle();
				if(isNumber(currentChar)) {
					int div = 10;
					while(isNumber(currentChar)) {
						num += (currentChar - 48) / div;
						div *= 10;

						acceptSingle();
					}

					skipSpace();
					if(neg) {
						num = -num;
					}
					return std::make_shared<JsonNumber>(num);
				}
			} else {
				skipSpace();
				if(neg) {
					num = -num;
				}
				return std::make_shared<JsonNumber>(num);
			}
		}

		auto msg = "Unexpected symbol "s + (char)currentChar + " at (" + std::to_string(line) + "," + std::to_string(row) + ") expected number";
		throw std::runtime_error(msg.c_str());
	}
	std::shared_ptr<JsonBool> parseBool() {
		// todo: improve
		bool val;
		if(currentChar == 't') {
			accept('t');
			accept('r');
			accept('u');
			accept('e');
			
			val = true;
		} else {
			accept('f');
			accept('a');
			accept('l');
			accept('s');
			accept('e');

			val = false;
		}

		skipSpace();
		return std::make_shared<JsonBool>(val);
	}
	std::shared_ptr<JsonBase> parseNull() {
		// todo: improve
		accept('n');
		accept('u');
		accept('l');
		accept('l');

		skipSpace();
		return nullptr;
	}
};

std::ostream& operator<<(std::ostream& stream, const Json& json) {
	if(json.root == nullptr) {
		stream << "null";
	} else {
		json.root->print(stream);
	}
	return stream;
}

std::istream& operator>>(std::istream& stream, Json& json) {
	JsonParser parser(stream);
	json.root = parser.parse();

	return stream;
}

bool Json::contains(const std::string& str) const {
	if(auto obj = dynamic_cast<JsonObject*>(root.get())) {
		return obj->elements.count(str);
	}
	return false;
}
