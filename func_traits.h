#pragma once

#include <functional>
#include <utility>
#include <memory>


template <typename T>
struct FunctionTraits : FunctionTraits<decltype(&T::operator())> {};



/*! \brief Характеристика функции
 *
 *  https://github.com/kennytm/utils/blob/master/traits.hpp
 */
template <typename T_Result, typename... T_Args>
struct FunctionTraits<T_Result(T_Args...)> {
 public:
  /*! \brief Возвращает тип результата вызова функции
   */
  using ResultType = T_Result;
  /*! \brief Тип функции как F(T...)
   */
  typedef ResultType FunctionType(T_Args...);
  /*! \brief Арность (кол-во аргументов) функции
   *
   */
  enum { Arity = sizeof...(T_Args) };
  /*! \brief Тип аргументов функции
   *
   *  Пример:
   *
   *  \code
   *   FunctionTraits<int(int, double)>::Arg<0>::Type // int
   *   FunctionTraits<int(int, double)>::Arg<1>::Type // double
   *  \encode
   */
  template <size_t i>
  struct Arg {
    using Type = typename std::tuple_element<i, std::tuple<T_Args...>>::type;
  };
};

template <typename T_Result, typename... T_Args>
struct FunctionTraits<T_Result (*)(T_Args...)> : public FunctionTraits<T_Result(T_Args...)> {};
template <typename T_Function>
struct FunctionTraits<std::function<T_Function>> : public FunctionTraits<T_Function> {};
template <typename T_Class, typename T_Result, typename... T_Args>
struct FunctionTraits<T_Result (T_Class::*)(T_Args...)> : public FunctionTraits<T_Result(T_Args...)> {};
template <typename T_Class, typename T_Result, typename... T_Args>
struct FunctionTraits<T_Result (T_Class::*)(T_Args...) const> : public FunctionTraits<T_Result(T_Args...)> {};
template <typename T_Class, typename T_Result, typename... T_Args>
struct FunctionTraits<T_Result (T_Class::*)(T_Args...) volatile> : public FunctionTraits<T_Result(T_Args...)> {};
template <typename T_Class, typename T_Result, typename... T_Args>
struct FunctionTraits<T_Result (T_Class::*)(T_Args...) const volatile> : public FunctionTraits<T_Result(T_Args...)> {};

template <typename T>
struct FunctionTraits<T&> : public FunctionTraits<T> {};
template <typename T>
struct FunctionTraits<const T&> : public FunctionTraits<T> {};
template <typename T>
struct FunctionTraits<volatile T&> : public FunctionTraits<T> {};
template <typename T>
struct FunctionTraits<const volatile T&> : public FunctionTraits<T> {};
template <typename T>
struct FunctionTraits<T&&> : public FunctionTraits<T> {};
template <typename T>
struct FunctionTraits<const T&&> : public FunctionTraits<T> {};
template <typename T>
struct FunctionTraits<volatile T&&> : public FunctionTraits<T> {};
template <typename T>
struct FunctionTraits<const volatile T&&> : public FunctionTraits<T> {};


