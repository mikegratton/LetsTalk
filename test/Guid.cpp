#include <cstdlib>
#include <iostream>

#include "LetsTalk/LetsTalk.hpp"
#include "doctest.h"

TEST_CASE("Guid.Conversion")
{
    lt::Guid myGuid;
    int v1 = rand();
    int v2 = rand();
    int v3 = rand();
    int v4 = rand();
    memcpy(&myGuid.data[0], &v1, sizeof(int));
    memcpy(&myGuid.data[1 * sizeof(int)], &v2, sizeof(int));
    memcpy(&myGuid.data[2 * sizeof(int)], &v3, sizeof(int));
    memcpy(&myGuid.data[3 * sizeof(int)], &v4, sizeof(int));
    myGuid.sequence = 5;

    auto fastId = lt::toSampleId(myGuid);
    for (int i = 0; i < 12; i++) { CHECK(myGuid.data[i] == fastId.writer_guid().guidPrefix.value[i]); }
    for (int i = 0; i < 4; i++) { CHECK(myGuid.data[12 + i] == fastId.writer_guid().entityId.value[i]); }
    CHECK(myGuid.sequence == fastId.sequence_number().to64long());

    // And back...
    auto reverse = lt::toLetsTalkGuid(fastId);
    CHECK(reverse == myGuid);
}

TEST_CASE("Guid.Bad")
{
    lt::Guid myGuid;
    int v1 = rand();
    int v2 = rand();
    int v3 = rand();
    int v4 = rand();
    memcpy(&myGuid.data[0], &v1, sizeof(int));
    memcpy(&myGuid.data[1 * sizeof(int)], &v2, sizeof(int));
    memcpy(&myGuid.data[2 * sizeof(int)], &v3, sizeof(int));
    memcpy(&myGuid.data[3 * sizeof(int)], &v4, sizeof(int));
    myGuid.sequence = 0;
    lt::Guid badGuid = myGuid.makeBadVersion();
    std::cout << "Good: " << myGuid.sequence << ", bad: " << badGuid.sequence << "\n";
    CHECK(badGuid.isBadVersionOf(myGuid));
}
