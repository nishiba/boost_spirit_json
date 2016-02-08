// boost_spirit_json.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <iostream>
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


Object load(const std::string& buffer) {
	JsonGrammar<decltype(buffer.begin())> g;
	Object result;
	auto begin = buffer.begin();
	auto end = buffer.end();
	bool isSuccess = qi::phrase_parse(begin, end, g, boost::spirit::qi::ascii::space, result);
	std::cout << (isSuccess && (begin == end) ? "OK" : "Fail") << std::endl;
	return result;
}


std::string serialize(const Component& c);

struct Serialize : public boost::static_visitor<std::string> {
public:
	std::string operator()(const boost::blank& x) const { return ""; }
	std::string operator()(const double& x) const { return std::to_string(x); }
	std::string operator()(const bool& x) const { return std::to_string(x); }
	std::string operator()(const std::string& x) const { return '"' + x + '"'; }
	std::string operator()(const Null& x) const { return std::string("null"); }
	std::string operator()(const Pair& x) const { return x.name() + ":" + serialize(x.value()); }
	std::string operator()(const Array& x) const { 
		std::string v = "";
		for (const Component& c : x.objects()) {
			v += serialize(c) + ',';
		}
		if (v.size() != 0) {
			v = v.substr(0, v.size() - 1);
		}
		return '[' + v + ']';
	}
	std::string operator()(const Object& x) const { 
		std::string v = "";
		for (const std::pair<std::string, Component>& c : x.components()) {
			v += c.first + ':' + serialize(c.second) + ',';
		}
		if (v.size() != 0) {
			v = v.substr(0, v.size() - 1);
		}
		return '{' + v + '}';
	}
};

std::string serialize(const Component& c)
{
	return boost::apply_visitor(Serialize(), c);
}


int main()
{
	std::string buffer;
	while (std::cout << "$ ", std::getline(std::cin, buffer)) {
		std::cout << serialize(load(buffer)) << std::endl;
	}

    return 0;
}

