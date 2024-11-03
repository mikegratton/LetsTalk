// Copyright (C) 2015-2023 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "detail/align.hpp"

#include <doctest/doctest.h>

using namespace foonathan::memory;
using namespace detail;

TEST_CASE("detail::align_offset")
{
    auto ptr = reinterpret_cast<void*>(0);
    REQUIRE(align_offset(ptr, 1) == 0u);
    REQUIRE(align_offset(ptr, 16) == 0u);
    ptr = reinterpret_cast<void*>(1);
    REQUIRE(align_offset(ptr, 1) == 0u);
    REQUIRE(align_offset(ptr, 16) == 15u);
    ptr = reinterpret_cast<void*>(8);
    REQUIRE(align_offset(ptr, 4) == 0u);
    REQUIRE(align_offset(ptr, 8) == 0u);
    REQUIRE(align_offset(ptr, 16) == 8u);
    ptr = reinterpret_cast<void*>(16);
    REQUIRE(align_offset(ptr, 16) == 0u);
    ptr = reinterpret_cast<void*>(1025);
    REQUIRE(align_offset(ptr, 16) == 15u);
}

TEST_CASE("detail::is_aligned")
{
    auto ptr = reinterpret_cast<void*>(0);
    REQUIRE(is_aligned(ptr, 1));
    REQUIRE(is_aligned(ptr, 8));
    REQUIRE(is_aligned(ptr, 16));
    ptr = reinterpret_cast<void*>(1);
    REQUIRE(is_aligned(ptr, 1));
    REQUIRE(!is_aligned(ptr, 16));
    ptr = reinterpret_cast<void*>(8);
    REQUIRE(is_aligned(ptr, 1));
    REQUIRE(is_aligned(ptr, 4));
    REQUIRE(is_aligned(ptr, 8));
    REQUIRE(!is_aligned(ptr, 16));
    ptr = reinterpret_cast<void*>(16);
    REQUIRE(is_aligned(ptr, 1));
    REQUIRE(is_aligned(ptr, 8));
    REQUIRE(is_aligned(ptr, 16));
    ptr = reinterpret_cast<void*>(1025);
    REQUIRE(is_aligned(ptr, 1));
    REQUIRE(!is_aligned(ptr, 16));
}

TEST_CASE("detail::alignment_for")
{
    static_assert(max_alignment >= 8, "test case not working");
    REQUIRE(alignment_for(1) == 1);
    REQUIRE(alignment_for(2) == 2);
    REQUIRE(alignment_for(3) == 2);
    REQUIRE(alignment_for(4) == 4);
    REQUIRE(alignment_for(5) == 4);
    REQUIRE(alignment_for(6) == 4);
    REQUIRE(alignment_for(7) == 4);
    REQUIRE(alignment_for(8) == 8);
    REQUIRE(alignment_for(9) == 8);
    REQUIRE(alignment_for(100) == max_alignment);
}
