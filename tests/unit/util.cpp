#include <tst/set.hpp>
#include <tst/check.hpp>

#include <string>

#include "../../src/easyhttp/util.hpp"

using namespace std::string_literals;

namespace{
tst::set set("util", [](tst::suite& suite){
	suite.add("url_escaping", [](){
		auto res = easyhttp::escape("Hello World!");
		tst::check_eq(res, "Hello%20World%21"s, SL);
	});
});
}