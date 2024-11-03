// Copyright (C) 2015-2023 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "aligned_allocator.hpp"

#include <doctest/doctest.h>

#include "detail/align.hpp"
#include "allocator_storage.hpp"
#include "memory_stack.hpp"

using namespace foonathan::memory;

TEST_CASE("aligned_allocator")
{
    using allocator_t = aligned_allocator<allocator_reference<memory_stack<>>>;

    memory_stack<> stack(1024);
    stack.allocate(3, 1); // manual misalign

    allocator_t alloc(4u, stack);
    REQUIRE(alloc.min_alignment() == 4u);

    auto mem1 = alloc.allocate_node(16u, 1u);
    REQUIRE(detail::align_offset(mem1, 4u) == 0u);
    auto mem2 = alloc.allocate_node(16u, 8u);
    REQUIRE(detail::align_offset(mem2, 4u) == 0u);
    REQUIRE(detail::align_offset(mem2, 8u) == 0u);

    alloc.deallocate_node(mem2, 16u, 8u);
    alloc.deallocate_node(mem1, 16u, 1u);
}
