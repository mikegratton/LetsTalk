// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "memory_arena.hpp"

#include <doctest/doctest.h>

#include "static_allocator.hpp"

using namespace foonathan::memory;
using namespace detail;

TEST_CASE("detail::memory_block_stack")
{
    memory_block_stack stack;
    REQUIRE(stack.empty());

    static_allocator_storage<1024> memory;

    stack.push({&memory, 1024});
    REQUIRE(!stack.empty());

    auto top = stack.top();
    REQUIRE(top.memory >= static_cast<void*>(&memory));
    REQUIRE(top.size <= 1024);
    REQUIRE(is_aligned(top.memory, max_alignment));

    SUBCASE("pop")
    {
        auto block = stack.pop();
        REQUIRE(block.size == 1024);
        REQUIRE(block.memory == static_cast<void*>(&memory));
    }
    SUBCASE("steal_top")
    {
        memory_block_stack other;

        other.steal_top(stack);
        REQUIRE(stack.empty());
        REQUIRE(!other.empty());

        auto other_top = other.top();
        REQUIRE(other_top.memory >= static_cast<void*>(&memory));
        REQUIRE(other_top.size <= 1024);
        REQUIRE(is_aligned(other_top.memory, max_alignment));
    }

    static_allocator_storage<1024> a, b, c;
    stack.push({&a, 1024});
    stack.push({&b, 1024});
    stack.push({&c, 1024});

    SUBCASE("multiple pop")
    {
        auto block = stack.pop();
        REQUIRE(block.memory == static_cast<void*>(&c));
        block = stack.pop();
        REQUIRE(block.memory == static_cast<void*>(&b));
        block = stack.pop();
        REQUIRE(block.memory == static_cast<void*>(&a));
        block = stack.pop();
        REQUIRE(block.memory == static_cast<void*>(&memory));
    }
    SUBCASE("multiple steal_from")
    {
        memory_block_stack other;

        other.steal_top(stack);
        other.steal_top(stack);
        other.steal_top(stack);
        other.steal_top(stack);

        REQUIRE(stack.empty());

        auto block = other.pop();
        REQUIRE(block.memory == static_cast<void*>(&memory));
        block = other.pop();
        REQUIRE(block.memory == static_cast<void*>(&a));
        block = other.pop();
        REQUIRE(block.memory == static_cast<void*>(&b));
        block = other.pop();
        REQUIRE(block.memory == static_cast<void*>(&c));
    }
    SUBCASE("move")
    {
        memory_block_stack other = detail::move(stack);
        REQUIRE(stack.empty());
        REQUIRE(!other.empty());

        auto block = other.pop();
        REQUIRE(block.memory == static_cast<void*>(&c));

        stack = detail::move(other);
        REQUIRE(other.empty());
        REQUIRE(!stack.empty());

        block = stack.pop();
        REQUIRE(block.memory == static_cast<void*>(&b));
    }
}

template <std::size_t N>
struct test_block_allocator
{
    static_allocator_storage<1024> blocks[N];
    std::size_t                    i = 0;

    test_block_allocator(std::size_t) {}

    ~test_block_allocator()
    {
        REQUIRE(i == 0u);
    }

    memory_block allocate_block()
    {
        REQUIRE(i < N);
        return {&blocks[i++], 1024};
    }

    void deallocate_block(memory_block b)
    {
        REQUIRE(static_cast<void*>(&blocks[i - 1]) == b.memory);
        --i;
    }

    std::size_t next_block_size() const
    {
        return 1024;
    }
};

TEST_CASE("memory_arena w/ caching")
{
    using arena_type = memory_arena<test_block_allocator<10>>;
    SUBCASE("basic")
    {
        arena_type arena(1024);
        REQUIRE(arena.get_allocator().i == 0u);
        REQUIRE(arena.size() == 0u);
        REQUIRE(arena.capacity() == 0u);

        arena.allocate_block();
        REQUIRE(arena.get_allocator().i == 1u);
        REQUIRE(arena.size() == 1u);
        REQUIRE(arena.capacity() == 1u);

        arena.allocate_block();
        REQUIRE(arena.get_allocator().i == 2u);
        REQUIRE(arena.size() == 2u);
        REQUIRE(arena.capacity() == 2u);

        arena.deallocate_block();
        REQUIRE(arena.get_allocator().i == 2u);
        REQUIRE(arena.size() == 1u);
        REQUIRE(arena.capacity() == 2u);

        arena.allocate_block();
        REQUIRE(arena.get_allocator().i == 2u);
        REQUIRE(arena.size() == 2u);
        REQUIRE(arena.capacity() == 2u);

        arena.deallocate_block();
        arena.deallocate_block();
        REQUIRE(arena.get_allocator().i == 2u);
        REQUIRE(arena.size() == 0u);
        REQUIRE(arena.capacity() == 2u);

        arena.shrink_to_fit();
        REQUIRE(arena.get_allocator().i == 0u);
        REQUIRE(arena.size() == 0u);
        REQUIRE(arena.capacity() == 0u);

        arena.allocate_block();
        REQUIRE(arena.get_allocator().i == 1u);
        REQUIRE(arena.size() == 1u);
        REQUIRE(arena.capacity() == 1u);
    }
    SUBCASE("small arena")
    {
        arena_type small_arena(arena_type::min_block_size(1));
        REQUIRE(small_arena.get_allocator().i == 0u);
        REQUIRE(small_arena.size() == 0u);
        REQUIRE(small_arena.capacity() == 0u);

        small_arena.allocate_block();
        REQUIRE(small_arena.get_allocator().i == 1u);
        REQUIRE(small_arena.size() == 1u);
        REQUIRE(small_arena.capacity() == 1u);
    }
}

TEST_CASE("memory_arena w/o caching")
{
    using arena_type = memory_arena<test_block_allocator<10>, false>;
    SUBCASE("normal")
    {
        arena_type arena(1024);
        REQUIRE(arena.get_allocator().i == 0u);
        REQUIRE(arena.size() == 0u);
        REQUIRE(arena.capacity() == 0u);

        arena.allocate_block();
        REQUIRE(arena.get_allocator().i == 1u);
        REQUIRE(arena.size() == 1u);
        REQUIRE(arena.capacity() == 1u);

        arena.allocate_block();
        REQUIRE(arena.get_allocator().i == 2u);
        REQUIRE(arena.size() == 2u);
        REQUIRE(arena.capacity() == 2u);

        arena.deallocate_block();
        REQUIRE(arena.get_allocator().i == 1u);
        REQUIRE(arena.size() == 1u);
        REQUIRE(arena.capacity() == 1u);

        arena.allocate_block();
        REQUIRE(arena.get_allocator().i == 2u);
        REQUIRE(arena.size() == 2u);
        REQUIRE(arena.capacity() == 2u);

        arena.deallocate_block();
        arena.deallocate_block();
        REQUIRE(arena.get_allocator().i == 0u);
        REQUIRE(arena.size() == 0u);
        REQUIRE(arena.capacity() == 0u);

        arena.allocate_block();
        REQUIRE(arena.get_allocator().i == 1u);
        REQUIRE(arena.size() == 1u);
        REQUIRE(arena.capacity() == 1u);
    }
    SUBCASE("small arena")
    {
        arena_type small_arena(arena_type::min_block_size(1));
        REQUIRE(small_arena.get_allocator().i == 0u);
        REQUIRE(small_arena.size() == 0u);
        REQUIRE(small_arena.capacity() == 0u);

        small_arena.allocate_block();
        REQUIRE(small_arena.get_allocator().i == 1u);
        REQUIRE(small_arena.size() == 1u);
        REQUIRE(small_arena.capacity() == 1u);
    }
}

static_assert(
    std::is_same<growing_block_allocator<>,
                 foonathan::memory::make_block_allocator_t<growing_block_allocator<>>>::value,
    "");
static_assert(std::is_same<growing_block_allocator<>,
                           foonathan::memory::make_block_allocator_t<default_allocator>>::value,
              "");

template <class RawAlloc>
using block_wrapper = growing_block_allocator<RawAlloc>;

TEST_CASE("make_block_allocator")
{
    growing_block_allocator<heap_allocator> a1 = make_block_allocator<heap_allocator>(1024);
    REQUIRE(a1.next_block_size() == 1024);

    growing_block_allocator<heap_allocator> a2 =
        make_block_allocator<block_wrapper, heap_allocator>(1024);
    REQUIRE(a2.next_block_size() == 1024);
}

