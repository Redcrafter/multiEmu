#pragma once
#include <deque>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

class Json;

struct JsonBase {
	virtual ~JsonBase() = default;

	virtual void print(std::ostream& stream) const = 0;

	virtual Json& operator[](const char* name);
	virtual Json& operator[](const std::string& name);
	virtual Json& operator[](size_t pos);

	virtual operator std::string() const;

	virtual operator double() const;
	virtual operator bool() const;
};

struct JsonObject final : public JsonBase {
	std::map<std::string, Json> elements;

	JsonObject(const std::map<std::string, Json>& elements) : elements(elements) {}
	~JsonObject() override = default;

	auto begin() { return elements.begin(); }
	auto end() { return elements.end(); }

	void print(std::ostream& stream) const override;

	Json& operator[](const char* name) override { return elements[std::string(name)]; }
	Json& operator[](const std::string& name) override { return elements[name]; }

	operator bool() const override { return true; }
};

struct JsonArray final : public JsonBase {
	std::vector<Json> elements;

	JsonArray(const std::vector<Json>& elements) : elements(elements) {}
	~JsonArray() override = default;

	auto begin() { return elements.begin(); }
	auto end() { return elements.end(); }

	void print(std::ostream& stream) const override;

	Json& operator[](size_t pos) override { return elements[pos]; }

	operator bool() const override { return true; }
};

struct JsonString : public JsonBase {
	const std::string value;

	JsonString(std::string value) : value(std::move(value)) {}
	~JsonString() override = default;

	void print(std::ostream& stream) const override;

	operator std::string() const override { return value; }
};

struct JsonNumber : public JsonBase {
	const double value;

	JsonNumber(double value) : value(value) {}
	~JsonNumber() override = default;

	void print(std::ostream& stream) const override;

	operator double() const override { return value; }
};

struct JsonBool : public JsonBase {
	const bool value;

	JsonBool(bool value) : value(value) {}
	~JsonBool() override = default;

	void print(std::ostream& stream) const override;

	operator bool() const override { return value; }
};

class Json {
	std::shared_ptr<JsonBase> root = nullptr;

  public:
	Json() = default;

	template<typename T>
	Json(std::vector<T> val) {
		std::vector<Json> elements;
		for(auto item : val) {
			elements.push_back(item);
		}
		root = std::make_shared<JsonArray>(elements);
	}

	template<typename T>
	Json(std::deque<T> val) {
		std::vector<Json> elements;
		for(auto item : val) {
			elements.push_back(item);
		}
		root = std::make_shared<JsonArray>(elements);
	}

	template<typename K, typename V>
	Json(std::map<K, V> val) {
		if constexpr(std::is_same<K, std::string>::value) {
			std::map<std::string, Json> elements;
			for(auto item : val) {
				elements[item.first] = item.second;
			}
			root = std::make_shared<JsonObject>(elements);
		} else {
			std::vector<Json> elements;
			for(auto item : val) {
				std::vector<Json> sub;
				sub.push_back(item.first);
				sub.push_back(item.second);
				elements.emplace_back(sub);
			}
			root = std::make_shared<JsonArray>(elements);
		}
	}

	Json(std::initializer_list<std::pair<const std::string, Json>> list) { root = std::make_shared<JsonObject>(list); }
	// Json(std::initializer_list<Json> list) { root = std::make_shared<JsonArray>(list); }

	Json(const std::shared_ptr<JsonBase> el) : root(el) {}

	Json(const char* str) { root = std::make_shared<JsonString>(str); }
	Json(const std::string& str) { root = std::make_shared<JsonString>(str); }

	Json(bool val) { root = std::make_shared<JsonBool>(val); }

	template<class T>
	Json(T val) {
		static_assert(std::is_arithmetic<T>::value || std::is_enum<T>::value);
		if constexpr(std::is_arithmetic<T>::value) {
			root = std::make_shared<JsonNumber>(val);
		} else if constexpr(std::is_enum<T>::value) {
			root = std::make_shared<JsonNumber>((std::underlying_type<T>::type)(val));
		}
	}

	bool contains(const std::string& str) const;

	JsonArray* asArray() { return dynamic_cast<JsonArray*>(root.get()); }
	JsonObject* asObject() { return dynamic_cast<JsonObject*>(root.get()); }

	bool tryGet(std::string& out) noexcept {
		if(auto ref = dynamic_cast<JsonString*>(root.get())) {
			out = ref->value;
			return true;
		}
		return false;
	}
	bool tryGet(bool& out) noexcept {
		if(auto ref = dynamic_cast<JsonBool*>(root.get())) {
			out = ref->value;
			return true;
		}
		return false;
	}
	template<typename T>
	bool tryGet(std::vector<T>& out) noexcept {
		if(auto ref = dynamic_cast<JsonArray*>(root.get())) {
			out.clear();

			T t;
			for(auto& item : ref->elements) {
				if(!item.tryGet(t)) {
					return false;
				}
				out.push_back(t);
			}
			return true;
		}
		return false;
	}
	template<typename T>
	bool tryGet(std::map<std::string, T>& out) noexcept {
		if(auto ref = dynamic_cast<JsonObject*>(root.get())) {
			out.clear();
			T t;
			for(auto& item : ref->elements) {
				if(!item.second.tryGet(t)) {
					return false;
				}
				out[item.first] = t;
			}
			return true;
		}
		return false;
	}

	template<typename T>
	bool tryGet(T& out) noexcept {
		// too lazy to make a function for each arithmetic type
		static_assert(std::is_arithmetic_v<T>);
		if(auto ref = dynamic_cast<JsonNumber*>(root.get())) {
			out = ref->value;
			return true;
		}
		return false;
	}

	Json& operator[](const char* name) { return (*root)[name]; }
	Json& operator[](const std::string& name) { return (*root)[name]; }
	Json& operator[](size_t pos) { return (*root)[pos]; }
	Json& operator[](int pos) { return (*root)[(size_t)pos]; }

	template<typename T>
	operator std::vector<T>() const {
		if(auto arr = dynamic_cast<JsonArray*>(root.get())) {
			std::vector<T> ret;
			for(auto item : arr->elements) {
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
	operator bool() const { return *root; }

	friend std::ostream& operator<<(std::ostream& stream, const Json& json);
	friend std::istream& operator>>(std::istream& stream, Json& json);
};
