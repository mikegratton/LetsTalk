#pragma once
#include <memory>
#include <type_traits>

#include "Guid.hpp"
namespace lt {

template <typename... Ts>
struct make_void {
  typedef void type;
};

template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

template <class C, class T, class = void>
struct wants_guids : std::false_type {
};

template <class C, class T>
struct wants_guids<C, T,
                   void_t<decltype(std::declval<C>().operator()(std::declval<T const&>(), std::declval<Guid const&>(),
                                                                std::declval<Guid const&>()))>> : std::true_type {
};

template <class C, class T, class D = std::default_delete<T>, class = void>
struct wants_uptr : std::false_type {
};

template <class C, class T, class D>
struct wants_uptr<C, T, D, void_t<decltype(std::declval<C>().operator()(std::declval<std::unique_ptr<T, D>>()))>>
    : std::true_type {
};

template <class C, class T, class D = std::default_delete<T>, class = void>
struct wants_uptr_with_guid : std::false_type {
};

template <class C, class T, class D>
struct wants_uptr_with_guid<
    C, T, D,
    void_t<decltype(std::declval<C>().operator()(std::declval<std::unique_ptr<T, D>>(), std::declval<Guid const&>(),
                                                 std::declval<Guid const&>()))>> : std::true_type {
};

struct wants_guid_tag {
};
struct plain_tag {
};
struct uptr_tag {
};
struct uptr_with_guid_tag {
};

template <bool B, class T, class F>
using conditional_t = typename std::conditional<B, T, F>::type;

template <class C, class T, class D = std::default_delete<T>>
struct functor_tagger {
  using type =
      conditional_t<wants_guids<C, T>::value, wants_guid_tag,
                    conditional_t<wants_uptr<C, T, D>::value, uptr_tag,
                                  conditional_t<wants_uptr_with_guid<C, T, D>::value, uptr_with_guid_tag, plain_tag>>>;
};

}  // namespace lt
