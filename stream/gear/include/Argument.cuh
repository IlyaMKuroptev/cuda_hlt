#pragma once

#include <tuple>
#include "Common.cuh"

template<int I, typename T>
struct Argument {
  constexpr static int i = I;
  T type_obj;

  Argument() = default;
};

/**
 * @brief Helper class to generate arguments based on
 *        the information provided by the base_pointer and offsets
 */
template<typename T>
struct StaticArgumentGenerator {
  T& arguments;
  char* base_pointer;
  std::vector<uint> offsets;

  StaticArgumentGenerator(T& param_arguments,
    char* param_base_pointer,
    std::vector<uint> param_offsets)
  : arguments(param_arguments),
    base_pointer(param_base_pointer),
    offsets(param_offsets) {}

  template<unsigned I>
  auto generate()
  -> decltype(std::get<I>(arguments).type_obj)* {
    auto& argument = std::get<I>(arguments);
    auto pointer = base_pointer + offsets[I];
    return reinterpret_cast<decltype(argument.type_obj)*>(pointer);
  }
};

/**
 * @brief Helper class to generate arguments based on
 *        the information provided by the base_pointer and offsets
 */
template<typename T>
struct DynamicArgumentGenerator {
  T& arguments;
  char* base_pointer;

  DynamicArgumentGenerator(T& param_arguments,
    char* param_base_pointer)
  : arguments(param_arguments),
    base_pointer(param_base_pointer) {}

  template<unsigned I>
  auto generate(const std::array<uint, std::tuple_size<T>::value>& offsets)
  -> decltype(std::get<I>(arguments).type_obj)* {
    auto& argument = std::get<I>(arguments);
    auto pointer = base_pointer + offsets.at(I);
    return reinterpret_cast<decltype(argument.type_obj)*>(pointer);
  }

  template<unsigned I>
  size_t size(const size_t s) {
    return s * sizeof(std::get<I>(arguments).type_obj);
  }
};

/**
 * @brief Generates a std::vector with the sizes of all arguments,
 *        taking account of their types, in order
 */
template<typename T, unsigned long... Is>
std::vector<size_t> generate_argument_sizes_impl(
  const T& tuple,
  std::index_sequence<Is...>
) {
  return {std::get<Is>(tuple).size * sizeof(std::get<Is>(tuple).type_obj)...};
}

template<typename T>
std::vector<size_t> generate_argument_sizes(const T& tuple) {
  using indices = typename tuple_indices<T>::type;
  return generate_argument_sizes_impl(tuple, indices());
}

/**
 * @brief Generates a std::vector with the names of all arguments.
 */
template<typename T, unsigned long... Is>
std::vector<std::string> generate_argument_names_impl(
  const T& tuple,
  std::index_sequence<Is...>
) {
  return {std::get<Is>(tuple).name...};
}

template<typename T>
std::vector<std::string> generate_argument_names(const T& tuple) {
  using indices = typename tuple_indices<T>::type;
  return generate_argument_names_impl(tuple, indices());
}
