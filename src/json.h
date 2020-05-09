#pragma once
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <ostream>
#include <initializer_list>
#include <deque>
#include <valarray>
#include <iterator>

class Json;

class JsonBase {
public:
	virtual Json& operator[](const char* name) { throw std::runtime_error("not allowed"); }
	virtual Json& operator[](std::string& name) { throw std::runtime_error("not allowed"); }
	virtual Json& operator[](size_t pos) { throw std::runtime_error("not allowed"); }

	virtual operator std::string() const { throw std::runtime_error("not allowed"); }
	virtual operator double() const { throw std::runtime_error("not allowed"); }
	virtual operator int() const { throw std::runtime_error("not allowed"); }
	virtual operator uint64_t() const { throw std::runtime_error("not allowed"); }
	virtual operator bool() const { throw std::runtime_error("not allowed"); }
	virtual void print(std::ostream& stream) const = 0;
};

class JsonObject : public JsonBase {
public:
	std::map<std::string, Json> elements;

	JsonObject(const std::map<std::string, Json>& elements) : elements(elements) { }

	void print(std::ostream& stream) const;

	std::map<std::string, Json>::iterator begin() { return elements.begin(); }
	std::map<std::string, Json>::iterator end() { return elements.end(); }

	Json& operator[](const char* name) override { return elements[std::string(name)]; }
	Json& operator[](std::string& name) override { return elements[name]; }
	Json& operator[](size_t pos) override { return elements[std::to_string(pos)]; }
	virtual operator bool() const { return true; }
};
class JsonArray : public JsonBase {
public:
	std::vector<Json> elements;

	JsonArray(const std::vector<Json>& elements) : elements(elements) { }
	
	void print(std::ostream& stream) const;

	std::vector<Json>::iterator begin() { return elements.begin(); }
	std::vector<Json>::iterator end() { return elements.end(); }
	
	Json& operator[](size_t pos) override { return elements[pos]; }
	virtual operator bool() const { return true; }

};
class JsonString : public JsonBase {
	std::string value;
public:
	JsonString(const std::string& value) : value(value) { }

	void print(std::ostream& stream) const;

	virtual operator std::string() const { return value; }
	virtual operator double() const { return std::stod(value); }
	virtual operator int() const { return std::stoi(value); }
	virtual operator bool() const { return !value.empty(); }

	friend struct JsonParser;
};
class JsonNumber : public JsonBase {
	double value;
public:
	JsonNumber(double value) : value(value) { }

	void print(std::ostream& stream) const;

	virtual operator std::string() const { return std::to_string(value); }
	virtual operator double() const { return value; }
	virtual operator int() const { return value; }
	virtual operator uint64_t() const { return value; }
	virtual operator bool() const { return value != 0; }
};
class JsonBool : public JsonBase {
	bool value;
public:
	JsonBool(bool value) : value(value) { }

	void print(std::ostream& stream) const;

	virtual operator std::string() const { return value ? "true" : "false"; }
	virtual operator double() const { return value; }
	virtual operator int() const { return value; }
	virtual operator bool() const { return value; }
};

class Json {
	std::shared_ptr<JsonBase> root = nullptr;
public:
	Json() { }
	
	template <typename T>
	Json(std::vector<T> val) {
		std::vector<Json> elements;
		for(auto item : val) { elements.push_back(item); }
		root = std::make_shared<JsonArray>(elements);
	}

	template <typename T>
	Json(std::deque<T> val) {
		std::vector<Json> elements;
		for(auto item : val) { elements.push_back(item); }
		root = std::make_shared<JsonArray>(elements);
	}

	template <typename K, typename V>
	Json(std::map<K, V> val) {
		if constexpr(std::is_same<K, std::string>::value) {
			std::map<std::string, Json> elements;
			for(auto item : val) { elements[item.first] = item.second; }
			root = std::make_shared<JsonObject>(elements);
		} else if constexpr(std::is_enum<K>::value && std::is_enum<V>::value) {
			typedef typename std::underlying_type<K>::type kt;
			typedef typename std::underlying_type<V>::type vt;

			std::vector<Json> elements;
			for(auto item : val) {
				std::vector<Json> sub;
				sub.push_back((kt)item.first);
				sub.push_back((vt)item.second);
				elements.push_back(sub);
			}
			root = std::make_shared<JsonArray>(elements);
		} else if constexpr(std::is_enum<K>::value) {
			typedef typename std::underlying_type<K>::type kt;

			std::vector<Json> elements;
			for(auto item : val) {
				std::vector<Json> sub;
				sub.push_back((kt)item.first);
				sub.push_back(item.second);
				elements.push_back(sub);
			}
			root = std::make_shared<JsonArray>(elements);
		} else if constexpr(std::is_enum<V>::value) {
			typedef typename std::underlying_type<V>::type vt;

			std::vector<Json> elements;
			for(auto item : val) {
				std::vector<Json> sub;
				sub.push_back(item.first);
				sub.push_back((vt)item.second);
				elements.push_back(sub);
			}
			root = std::make_shared<JsonArray>(elements);
		} else {
			std::vector<Json> elements;
			for(auto item : val) {
				std::vector<Json> sub;
				sub.push_back(item.first);
				sub.push_back(item.second);
				elements.push_back(sub);
			}
			root = std::make_shared<JsonArray>(elements);
		}
	}

	Json(std::initializer_list<std::pair<const std::string, Json>> list) {
		root = std::make_shared<JsonObject>(list);
	}

	Json(const std::shared_ptr<JsonBase> el) : root(el) { }
	Json(const std::string& str) { root = std::make_shared<JsonString>(str); }
	Json(const char* str) { root = std::make_shared<JsonString>(str); }

	Json(bool val) { root = std::make_shared<JsonBool>(val); }

	Json(uint8_t val) { root = std::make_shared<JsonNumber>(val); }
	Json(uint16_t val) { root = std::make_shared<JsonNumber>(val); }
	Json(uint32_t val) { root = std::make_shared<JsonNumber>(val); }
	Json(uint64_t val) { root = std::make_shared<JsonNumber>(val); }

	Json(int8_t val) { root = std::make_shared<JsonNumber>(val); }
	Json(int16_t val) { root = std::make_shared<JsonNumber>(val); }
	Json(int32_t val) { root = std::make_shared<JsonNumber>(val); }
	Json(int64_t val) { root = std::make_shared<JsonNumber>(val); }

	Json(float val) { root = std::make_shared<JsonNumber>(val); }
	Json(double val) { root = std::make_shared<JsonNumber>(val); }


	bool contains(const std::string& str) const;

	JsonArray* asArray() { return dynamic_cast<JsonArray*>(root.get()); }
	JsonObject* asObject() { return dynamic_cast<JsonObject*>(root.get()); }

	Json& operator[](const char* name) { return (*root)[name]; }
	Json& operator[](std::string& name) { return (*root)[name]; }
	Json& operator[](size_t pos) { return (*root)[pos]; }
	Json& operator[](int pos) { return (*root)[(size_t)pos]; }

	template<typename T>
	operator std::vector<T>() const {
		if(auto arr = dynamic_cast<JsonArray*>(root.get())) {
			std::vector<T> ret;
			for(auto item: arr->elements) {
				ret.push_back(item);
			}
			return ret;
		} else {
			throw std::runtime_error("not allowed");
		}
	}
	template<typename V>
	operator std::map<std::string, V>() const {
		if(auto arr = dynamic_cast<JsonObject*>(root.get())) {
			std::map<std::string, V> ret;
			for(auto item : arr->elements) {
				ret[item.first] = (V)item.second;
			}
			return ret;
		} else {
			throw std::runtime_error("not allowed");
		}
	}
	operator std::string() const { return *root; }
	operator double() const { return *root; }
	operator int() const { return *root; }
	virtual operator uint64_t() const { return *root; }
	operator bool() const { return *root; }

	friend std::ostream& operator<<(std::ostream& stream, const Json& json);
	friend std::istream& operator>>(std::istream& stream, Json& json);
};
