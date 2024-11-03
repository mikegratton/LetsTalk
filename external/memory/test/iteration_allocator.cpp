// Copyright (C) 2015-2023 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "iteration_allocator.hpp"

#include <doctest/doctest.h>

#include "allocator_storage.hpp"
#include "test_allocator.hpp"

using namespace foonathan::memory;

TEST_CASE("iteration_allocator")
{
    SUBCASE("basic")
    {
        test_allocator                                              alloc;
        iteration_allocator<2, allocator_reference<test_allocator>> iter_alloc(100, alloc);
        REQUIRE(alloc.no_allocated() == 1u);
        REQUIRE(iter_alloc.max_iterations() == 2u);
        REQUIRE(iter_alloc.cur_iteration() == 0u);
        REQUIRE(iter_alloc.capacity_left(0u) == 50);
        REQUIRE(iter_alloc.capacity_left(1u) == 50);

        iter_alloc.allocate(10, 1);
        REQUIRE(iter_alloc.capacity_left() < 50);
        iter_alloc.allocate(4, 4);
        REQUIRE(iter_alloc.capacity_left() < 50);

        REQUIRE(iter_alloc.capacity_left(1u) == 50);
        iter_alloc.next_iteration();
        REQUIRE(iter_alloc.cur_iteration() == 1u);
        REQUIRE(iter_alloc.capacity_left() == 50);
        REQUIRE(iter_alloc.capacity_left(0u) < 50);

        iter_alloc.allocate(10, 1);
        REQUIRE(iter_alloc.capacity_left() < 50);

        iter_alloc.next_iteration();
        REQUIRE(iter_alloc.cur_iteration() == 0u);
        REQUIRE(iter_alloc.capacity_left() == 50);
        REQUIRE(iter_alloc.capacity_left(1u) < 50);

        iter_alloc.next_iteration();
        REQUIRE(iter_alloc.cur_iteration() == 1u);
        REQUIRE(iter_alloc.capacity_left() == 50);
    }
    SUBCASE("overaligned")
    {
        test_allocator                                              alloc;
        iteration_allocator<1, allocator_reference<test_allocator>> iter_alloc(100, alloc);

        auto align = 2 * detail::max_alignment;
        auto mem   = iter_alloc.allocate(align, align);
        REQUIRE(detail::is_aligned(mem, align));
    }
}
