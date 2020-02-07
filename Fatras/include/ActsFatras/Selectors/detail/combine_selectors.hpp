// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace ActsFatras {
namespace detail {

/// Combine multiple selectors with a configurable combine function.
template <bool Initial, typename Combine, typename... Selectors>
class CombineSelectors {
  static_assert(0u < sizeof...(Selectors),
                "Must combine at least one selector");

 public:
  /// Call all configured selectors and combine the result.
  ///
  /// @param[in] object the object that will be selected
  /// @return a boolean indicating the combined selection decision
  ///
  /// @tparam T the type of the selected object
  template <typename T>
  bool operator()(const T &thing) const {
    static_assert(
        (true && ... && std::is_same_v<bool, decltype(Selectors()(thing))>),
        "Not all selectors conform to the expected interface (bool)(const T&)");
    return impl(thing, std::index_sequence_for<Selectors...>(), Combine());
  }

  /// Access a specific selector by index.
  template <size_t I>
  std::tuple_element_t<I, std::tuple<Selectors...>> &get() {
    return std::get<I>(selectors);
  }
  /// Access a specific selector by type.
  template <typename Selector>
  Selector &get() {
    return std::get<Selector>(selectors);
  }

 private:
  std::tuple<Selectors...> selectors;

  template <typename T, std::size_t... Is>
  bool impl(const T &thing, std::index_sequence<Is...>, Combine combine) const {
    // compute status for all selectors
    bool status[] = {std::get<Is>(selectors)(thing)...};
    // reduce over the combine function with configured initial value
    bool ret = Initial;
    for (bool value : status) {
      ret = combine(ret, value);
    }
    return ret;
  }
};

}  // namespace detail
}  // namespace ActsFatras
