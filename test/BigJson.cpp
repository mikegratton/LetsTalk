#include <iostream>

#include "doctest.h"
#include "idl/Big.h"

TEST_CASE("BigJson")
{
    Big big;
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
    big.enum_thing(Second);
    big.bitset_thing().a(2);

    std::cout << big.toJson() << "\n";
}