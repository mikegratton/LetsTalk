#include <exception>
#include <iostream>

#include "doctest.h"
#include "idl/Big.hpp"
#include "idl/BigJsonSupport.hpp"

TEST_CASE("BigJson")
{
    modname::Big big;
    big.inner().index(1);
    big.inner().message("text");
    big.array()[0] = 1.0;
    big.array()[1] = 2.0;
    big.array()[2] = 3.0;
    big.array()[3] = 4.0;
    big.seq().push_back(7);
    big.bool_thing(true);
    std::vector<int> data;
    big.two_face().y(data);
    big.enum_thing(modname::Discriminator::Second);
    big.bitset_thing().a = 2;
    std::string text = modname::BigToJson(big);
    std::cout << text << "\n";
    CHECK(text.size() == strlen(text.c_str()));
    try {
        modname::Big big2 = modname::BigFromJson(text);
        std::string text2 = modname::BigToJson(big2);
        CHECK(text == text2);
    } catch (std::exception const& e) {
        std::cout << "THREW: " << e.what() << "\n";
    }
}
