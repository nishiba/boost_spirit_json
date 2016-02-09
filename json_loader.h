#pragma once



#include <string>
#include <vector>
#include <map>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/recursive_variant.hpp>


namespace qi = boost::spirit::qi;


class Null {};
class Pair;
class Array;
class Object;

typedef boost::variant<
	boost::blank,
	double,
	bool,
	std::string,
	Null,
	boost::recursive_wrapper<Pair>,
	boost::recursive_wrapper<Array>,
	boost::recursive_wrapper<Object>
> Component;

class Pair {
public:
	Pair() : _name(), _v() {}
	Pair(const std::string& name, const Component& v) : _name(name), _v(v) {}
public:
	const std::string& name() const { return _name; };
	const Component& value() const { return _v; }
private:
	std::string _name;
	Component _v;
};

class Object {
public:
	Object() : _v() {}
	Object(const std::vector<Pair>& v) : _v()
	{
		for (const Pair& p : v) { _v[p.name()] = p.value(); }
	}

	const std::map<std::string, Component>& components() const { return _v; }
private:
	std::map<std::string, Component> _v;
};

class Array {
public:
	Array() : _v() {}
	Array(const std::vector<Object>& v) : _v(v) {}
	const std::vector<Object>& objects() const { return _v; }
private:
	std::vector<Object> _v;
};

template <typename Iterator, typename Skip = boost::spirit::ascii::space_type>
class StringGrammar : public qi::grammar<Iterator, std::string(), Skip> {
public:
	StringGrammar() : StringGrammar::base_type(_str)
	{
		using namespace boost::spirit;
		using namespace boost::phoenix;
		_q = "\\\"";
		_e = "\\\\";
		_cstr = (qi::char_ - '"' - '\\')[_val = _1] | _q[_val = "\\\""] | _e[_val = "\\\\"];
		_str = '"' >> *_cstr[_val += _1] >> '"';
	}
private:
	qi::rule<Iterator, std::string(), Skip> _str, _cstr, _q, _e;
};


template <typename Iterator, typename Skip = boost::spirit::ascii::space_type>
class JsonGrammar : public qi::grammar<Iterator, Object(), Skip> {
public:
	JsonGrammar() : JsonGrammar::base_type(_object)
	{
		using namespace boost::spirit;
		using namespace boost::phoenix;
		_null = qi::lit("null")[_val = Null()];
		_factor = _str | bool_ | double_ | _object | _array | _null;
		_pair = (_str >> ':' >> _factor)[_val = makePair()];
		_array = '[' >> -(_object % ',')[_val = makeArray()] >> ']';
		_object = '{' >> -(_pair % ',')[_val = makeObject()] >> "}";
	}
private:
	StringGrammar<Iterator, Skip> _str;
	qi::rule<Iterator, Pair(), Skip> _pair;
	qi::rule<Iterator, Object(), Skip> _object;
	qi::rule<Iterator, Array(), Skip> _array;
	qi::rule<Iterator, Component(), Skip> _factor, _null;
private:
	auto makePair() {
		return boost::phoenix::bind(
			[](auto& name, auto& v) {return Pair(name, v); },
			boost::spirit::_1, boost::spirit::_2);
	}
	auto makeArray() {
		return boost::phoenix::bind(
			[](auto& p) { return Array(p); },
			boost::spirit::_1);
	}
	auto makeObject() {
		return boost::phoenix::bind(
			[](auto& p) { return Object(p); },
			boost::spirit::_1);
	}
};


inline Object load(const std::string& buffer) {
	JsonGrammar<decltype(buffer.begin())> g;
	Object result;
	auto begin = buffer.begin();
	auto end = buffer.end();
	bool isSuccess = qi::phrase_parse(begin, end, g, boost::spirit::qi::ascii::space, result);
	std::cout << (isSuccess && (begin == end) ? "OK" : "Fail") << std::endl;
	return result;
}
