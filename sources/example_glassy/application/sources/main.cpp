#include <stddef.h>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <tuple>
#include <type_traits>

namespace etl {

template <typename T>
class successor {
 public:
  typedef T successor_type;

  successor() : p_successor(nullptr) {}

  successor(successor_type& s) : p_successor(&s) {}

  void set_successor(successor_type& s) { p_successor = &s; }

  successor_type& get_successor() const { return *p_successor; }

  bool has_successor() const { return (p_successor != nullptr); }

 private:
  successor_type* p_successor;
};
}  // namespace etl

namespace etl {

class imemory_block_allocator : public successor<imemory_block_allocator> {
 public:
  imemory_block_allocator() {}

  virtual ~imemory_block_allocator() {}

 private:
  imemory_block_allocator(const etl::imemory_block_allocator&) = delete;
  imemory_block_allocator& operator=(const imemory_block_allocator&) = delete;
};
}  // namespace etl

namespace etl {

class exception {
 public:
  typedef const char* string_type;
  typedef int numeric_type;

  exception(string_type reason_, string_type file_, numeric_type line_)
      : reason_text(reason_), file_text(file_), line(line_) {}

  string_type what() const { return reason_text; }

  string_type file_name() const { return file_text; }

  numeric_type line_number() const { return line; }

 private:
  string_type reason_text;

  string_type file_text;

  numeric_type line;
};
}  // namespace etl

namespace etl {

template <typename TParameter>
class ifunction {
 public:
  typedef TParameter parameter_type;

  virtual ~ifunction() {}

  virtual void operator()(TParameter) const = 0;
};

template <>
class ifunction<void> {
 public:
  typedef void parameter_type;

  virtual ~ifunction() {}

  virtual void operator()() const = 0;
};

template <typename TObject, typename TParameter>
class function : public ifunction<TParameter> {
 public:
  typedef TObject object_type;
  typedef TParameter parameter_type;

  function(TObject& object_, void (TObject::*p_function_)(TParameter))
      : p_object(&object_), p_function(p_function_) {}

  virtual void operator()(TParameter data) const override {
    (p_object->*p_function)(data);
  }

 private:
  TObject* p_object;
  void (TObject::*p_function)(TParameter);
};

template <typename TObject>
class function<TObject, void> : public ifunction<void> {
 public:
  function(TObject& object_, void (TObject::*p_function_)(void))
      : p_object(&object_), p_function(p_function_) {}

  virtual void operator()() const override { (p_object->*p_function)(); }

 private:
  TObject* p_object;
  void (TObject::*p_function)();
};

template <typename TParameter>
class function<void, TParameter> : public ifunction<TParameter> {
 public:
  explicit function(void (*p_function_)(TParameter))
      : p_function(p_function_) {}

  virtual void operator()(TParameter data) const override {
    (*p_function)(data);
  }

 private:
  void (*p_function)(TParameter);
};

template <>
class function<void, void> : public ifunction<void> {
 public:
  explicit function(void (*p_function_)(void)) : p_function(p_function_) {}

  virtual void operator()() const override { (*p_function)(); }

 private:
  void (*p_function)();
};

template <typename TObject,
          typename TParameter,
          void (TObject::*Function)(TParameter)>
class function_mp : public ifunction<TParameter> {
 public:
  typedef TObject object_type;
  typedef TParameter parameter_type;

  explicit function_mp(TObject& object_) : p_object(&object_) {}

  virtual void operator()(TParameter data) const override {
    (p_object->*Function)(data);
  }

 private:
  TObject* p_object;
};

template <typename TObject, void (TObject::*Function)(void)>
class function_mv : public ifunction<void> {
 public:
  typedef TObject object_type;
  typedef void parameter_type;

  explicit function_mv(TObject& object_) : p_object(&object_) {}

  virtual void operator()() const override { (p_object->*Function)(); }

 private:
  TObject* p_object;
};

template <typename TObject,
          typename TParameter,
          TObject& Instance,
          void (TObject::*Function)(TParameter)>
class function_imp : public ifunction<TParameter> {
 public:
  typedef TObject object_type;
  typedef TParameter parameter_type;

  virtual void operator()(TParameter data) const override {
    (Instance.*Function)(data);
  }
};

template <typename TObject, TObject& Instance, void (TObject::*Function)(void)>
class function_imv : public ifunction<void> {
 public:
  typedef TObject object_type;
  typedef void parameter_type;

  virtual void operator()() const override { (Instance.*Function)(); }
};

template <typename TParameter, void (*Function)(TParameter)>
class function_fp : public ifunction<TParameter> {
 public:
  typedef TParameter parameter_type;

  function_fp() {}

  virtual void operator()(TParameter data) const override { (*Function)(data); }
};

template <void (*Function)(void)>
class function_fv : public ifunction<void> {
 public:
  typedef void parameter_type;

  function_fv() {}

  virtual void operator()() const override { (*Function)(); }
};

}  // namespace etl

namespace etl {

class error_handler {
 public:
  struct free_function : public etl::function<void, const etl::exception&> {
    explicit free_function(void (*p_function_)(const etl::exception&))
        : etl::function<void, const etl::exception&>(p_function_) {}
  };

  template <typename TObject>
  struct member_function
      : public etl::function<TObject, const etl::exception&> {
    member_function(TObject& object_,
                    void (TObject::*p_function_)(const etl::exception&))
        : etl::function<TObject, const etl::exception&>(object_, p_function_) {}
  };

  static void set_callback(ifunction<const etl::exception&>& f) {
    create((void*)(&f), ifunction_stub);
  }

  template <void (*Method)(const etl::exception&)>
  static void set_callback() {
    create(nullptr, function_stub<Method>);
  }

  template <typename T, void (T::*Method)(const etl::exception&)>
  static void set_callback(T& instance) {
    create((void*)(&instance), method_stub<T, Method>);
  }

  template <typename T, void (T::*Method)(const etl::exception&) const>
  static void set_callback(const T& instance) {
    create((void*)(&instance), const_method_stub<T, Method>);
  }

  template <typename T, T& Instance, void (T::*Method)(const etl::exception&)>
  static void set_callback() {
    create(method_instance_stub<T, Instance, Method>);
  }

  template <typename T,
            T const& Instance,
            void (T::*Method)(const etl::exception&) const>
  static void set_callback() {
    create(const_method_instance_stub<T, Instance, Method>);
  }

  static void error(const etl::exception& e) {
    invocation_element& invocation = get_invocation_element();

    if (invocation.stub != nullptr) {
      (*invocation.stub)(invocation.object, e);
    }
  }

 private:
  typedef void (*stub_type)(void* object, const etl::exception&);

  struct invocation_element {
    invocation_element() : object(nullptr), stub(nullptr) {}

    void* object;
    stub_type stub;
  };

  static invocation_element& get_invocation_element() {
    static invocation_element invocation;

    return invocation;
  }

  static void create(void* object, stub_type stub) {
    invocation_element& invocation = get_invocation_element();

    invocation.object = object;
    invocation.stub = stub;
  }

  static void create(stub_type stub) {
    invocation_element& invocation = get_invocation_element();

    invocation.object = nullptr;
    invocation.stub = stub;
  }

  template <typename T, void (T::*Method)(const etl::exception&)>
  static void method_stub(void* object, const etl::exception& e) {
    T* p = static_cast<T*>(object);
    return (p->*Method)(e);
  }

  template <typename T, void (T::*Method)(const etl::exception&) const>
  static void const_method_stub(void* object, const etl::exception& e) {
    T* const p = static_cast<T*>(object);
    return (p->*Method)(e);
  }

  template <typename T, T& Instance, void (T::*Method)(const etl::exception&)>
  static void method_instance_stub(void*, const etl::exception& e) {
    return (Instance.*Method)(e);
  }

  template <typename T,
            const T& Instance,
            void (T::*Method)(const etl::exception&) const>
  static void const_method_instance_stub(void*, const etl::exception& e) {
    (Instance.*Method)(e);
  }

  template <void (*Method)(const etl::exception&)>
  static void function_stub(void*, const etl::exception& e) {
    (Method)(e);
  }

  static void ifunction_stub(void* object, const etl::exception& e) {
    etl::ifunction<const etl::exception&>* p =
        static_cast<etl::ifunction<const etl::exception&>*>(object);
    p->operator()(e);
  }
};
}  // namespace etl

namespace etl {

template <typename...>
using void_t = void;
template <typename T, const T VALUE>
struct integral_constant : std::integral_constant<T, VALUE> {};

typedef integral_constant<bool, false> false_type;
typedef integral_constant<bool, true> true_type;

template <bool B>
using bool_constant = std::bool_constant<B>;
template <typename T>
struct negation : std::negation<T> {};

template <typename T>
inline constexpr bool negation_v = std::negation_v<T>;

template <typename T>
struct remove_reference : std::remove_reference<T> {};

template <typename T>
using remove_reference_t = typename std::remove_reference<T>::type;

template <typename T>
struct remove_pointer : std::remove_pointer<T> {};

template <typename T>
using remove_pointer_t = typename std::remove_pointer<T>::type;

template <typename T>
struct add_pointer : std::add_pointer<T> {};

template <typename T>
using add_pointer_t = typename std::add_pointer<T>::type;

template <typename T>
struct is_const : std::is_const<T> {};

template <typename T>
inline constexpr bool is_const_v = std::is_const_v<T>;

template <typename T>
struct remove_const : std::remove_const<T> {};

template <typename T>
using remove_const_t = typename std::remove_const<T>::type;

template <typename T>
struct add_const : std::add_const<T> {};

template <typename T>
using add_const_t = typename std::add_const<T>::type;

template <typename T>
struct is_volatile : std::is_volatile<T> {};

template <typename T>
inline constexpr bool is_volatile_v = std::is_volatile_v<T>;

template <typename T>
struct remove_volatile : std::remove_volatile<T> {};

template <typename T>
using remove_volatile_t = typename std::remove_volatile<T>::type;

template <typename T>
struct add_volatile : std::add_volatile<T> {};

template <typename T>
using add_volatile_t = typename std::add_volatile<T>::type;

template <typename T>
struct remove_cv : std::remove_cv<T> {};

template <typename T>
using remove_cv_t = typename std::remove_cv<T>::type;

template <typename T>
struct add_cv : std::add_cv<T> {};

template <typename T>
using add_cv_t = typename std::add_cv<T>::type;

template <typename T>
struct remove_cvref {
  typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type
      type;
};

template <typename T>
using remove_cvref_t = typename etl::remove_cvref<T>::type;

template <typename T>
struct is_integral : std::is_integral<T> {};

template <typename T>
inline constexpr bool is_integral_v = std::is_integral_v<T>;

template <typename T>
struct is_signed : std::is_signed<T> {};

template <typename T>
inline constexpr bool is_signed_v = std::is_signed_v<T>;

template <typename T>
struct is_unsigned : std::is_unsigned<T> {};

template <typename T>
inline constexpr bool is_unsigned_v = std::is_unsigned_v<T>;

template <typename T>
struct is_floating_point : std::is_floating_point<T> {};

template <typename T>
inline constexpr bool is_floating_point_v = std::is_floating_point_v<T>;

template <typename T1, typename T2>
struct is_same : std::is_same<T1, T2> {};

template <typename T1, typename T2>
inline constexpr bool is_same_v = std::is_same_v<T1, T2>;

template <typename T>
struct is_void : std::is_void<T> {};

template <typename T>
inline constexpr bool is_void_v = std::is_void_v<T>;

template <typename T>
struct is_arithmetic : std::is_arithmetic<T> {};

template <typename T>
inline constexpr bool is_arithmetic_v = std::is_arithmetic_v<T>;

template <typename T>
struct is_fundamental : std::is_fundamental<T> {};

template <typename T>
inline constexpr bool is_fundamental_v = std::is_fundamental_v<T>;

template <typename T>
struct is_compound : std::is_compound<T> {};

template <typename T>
inline constexpr bool is_compound_v = std::is_compound_v<T>;

template <typename T>
struct is_array : std::is_array<T> {};

template <typename T>
inline constexpr bool is_array_v = std::is_array_v<T>;

template <typename T>
struct is_pointer : std::is_pointer<T> {};

template <typename T>
inline constexpr bool is_pointer_v = std::is_pointer_v<T>;

template <typename T>
struct is_reference : std::is_reference<T> {};

template <typename T>
inline constexpr bool is_reference_v = std::is_reference_v<T>;

template <typename T>
struct is_lvalue_reference : std::is_lvalue_reference<T> {};

template <typename T>
inline constexpr bool is_lvalue_reference_v = std::is_lvalue_reference_v<T>;

template <typename T>
struct is_rvalue_reference : std::is_rvalue_reference<T> {};

template <typename T>
inline constexpr bool is_rvalue_reference_v = std::is_rvalue_reference_v<T>;

template <typename T>
struct is_pod : std::integral_constant<bool,
                                       std::is_standard_layout<T>::value &&
                                           std::is_trivial<T>::value> {};

template <typename T>
inline constexpr bool is_pod_v =
    std::is_standard_layout_v<T>&& std::is_trivial_v<T>;
template <bool B, typename T, typename F>
struct conditional {
  typedef T type;
};
template <typename T, typename F>
struct conditional<false, T, F> {
  typedef F type;
};

template <bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type;

template <typename T>
struct make_signed : std::make_signed<T> {};

template <typename T>
using make_signed_t = typename std::make_signed<T>::type;

template <typename T>
struct make_unsigned : std::make_unsigned<T> {};

template <typename T>
using make_unsigned_t = typename std::make_unsigned<T>::type;

template <bool B, typename T = void>
struct enable_if : std::enable_if<B, T> {};

template <bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

template <typename T, unsigned MAXN = 0U>
struct extent : std::extent<T, MAXN> {};

template <typename T, unsigned MAXN = 0U>
inline constexpr size_t extent_v = std::extent_v<T, MAXN>;

template <typename T>
struct remove_extent : std::remove_extent<T> {};

template <typename T>
using remove_extent_t = typename std::remove_extent<T>::type;

template <typename T>
struct remove_all_extents : std::remove_all_extents<T> {};

template <typename T>
using remove_all_extents_t = typename std::remove_all_extents<T>::type;

template <typename T>
struct rank : std::rank<T> {};

template <typename T>
inline constexpr size_t rank_v = std::rank_v<T>;

template <typename T>
struct decay : std::decay<T> {};

template <typename T>
using decay_t = typename std::decay<T>::type;

template <typename TBase, typename TDerived>
struct is_base_of : std::is_base_of<TBase, TDerived> {};

template <typename TBase, typename TDerived>
inline constexpr bool is_base_of_v = std::is_base_of_v<TBase, TDerived>;

template <typename T>
struct is_class : std::is_class<T> {};

template <typename T>
inline constexpr bool is_class_v = is_class<T>::value;

template <typename T>
struct add_lvalue_reference : std::add_lvalue_reference<T> {};

template <typename T>
using add_lvalue_reference_t = typename std::add_lvalue_reference<T>::type;

template <typename T>
struct add_rvalue_reference : std::add_rvalue_reference<T> {};

template <typename T>
using add_rvalue_reference_t = typename std::add_rvalue_reference<T>::type;

template <typename T>
typename std::add_rvalue_reference<T>::type declval() noexcept;

template <typename TFrom, typename TTo>
struct is_convertible : std::is_convertible<TFrom, TTo> {};

template <typename TFrom, typename TTo>
inline constexpr bool is_convertible_v = std::is_convertible_v<TFrom, TTo>;

template <typename T>
struct alignment_of : std::alignment_of<T> {};
template <>
struct alignment_of<void> : std::integral_constant<size_t, 0> {};
template <>
struct alignment_of<const void> : std::integral_constant<size_t, 0> {};

template <typename T>
inline constexpr size_t alignment_of_v = std::alignment_of_v<T>;

template <bool B, typename T, T TRUE_VALUE, T FALSE_VALUE>
struct conditional_integral_constant;

template <typename T, T TRUE_VALUE, T FALSE_VALUE>
struct conditional_integral_constant<true, T, TRUE_VALUE, FALSE_VALUE> {
  static_assert(etl::is_integral<T>::value, "Not an integral type");
  static const T value = TRUE_VALUE;
};

template <typename T, T TRUE_VALUE, T FALSE_VALUE>
struct conditional_integral_constant<false, T, TRUE_VALUE, FALSE_VALUE> {
  static_assert(etl::is_integral<T>::value, "Not an integral type");
  static const T value = FALSE_VALUE;
};

template <typename T, typename T1, typename... TRest>
struct is_one_of {
  static const bool value =
      etl::is_same<T, T1>::value || etl::is_one_of<T, TRest...>::value;
};

template <typename T, typename T1>
struct is_one_of<T, T1> {
  static const bool value = etl::is_same<T, T1>::value;
};

template <typename T, typename... TRest>
inline constexpr bool is_one_of_v = etl::is_one_of<T, TRest...>::value;

template <typename T>
struct types {
 private:
  typedef typename etl::remove_reference<typename etl::remove_cv<T>::type>::type
      type_t;

 public:
  typedef type_t type;
  typedef type_t& reference;
  typedef const type_t& const_reference;
  typedef type_t* pointer;
  typedef const type_t* const_pointer;
  typedef const type_t* const const_pointer_const;

  typedef type_t&& rvalue_reference;
};

template <typename T>
struct types<T*> {
 private:
  typedef typename etl::remove_reference<typename etl::remove_cv<T>::type>::type
      type_t;

 public:
  typedef type_t type;
  typedef type_t& reference;
  typedef const type_t& const_reference;
  typedef type_t* pointer;
  typedef const type_t* const_pointer;
  typedef const type_t* const const_pointer_const;

  typedef type_t&& rvalue_reference;
};

template <typename T>
struct types<T* const> {
 private:
  typedef typename etl::remove_reference<typename etl::remove_cv<T>::type>::type
      type_t;

 public:
  typedef type_t type;
  typedef type_t& reference;
  typedef const type_t& const_reference;
  typedef type_t* pointer;
  typedef const type_t* const_pointer;
  typedef const type_t* const const_pointer_const;

  typedef type_t&& rvalue_reference;
};

template <typename T>
struct types<T&> {
 private:
  typedef typename etl::remove_reference<typename etl::remove_cv<T>::type>::type
      type_t;

 public:
  typedef type_t type;
  typedef type_t& reference;
  typedef const type_t& const_reference;
  typedef type_t* pointer;
  typedef const type_t* const_pointer;
  typedef const type_t* const const_pointer_const;

  typedef type_t&& rvalue_reference;
};

template <typename T>
struct types<T&&> {
 private:
  typedef typename etl::remove_reference<typename etl::remove_cv<T>::type>::type
      type_t;

 public:
  typedef type_t type;
  typedef type_t& reference;
  typedef const type_t& const_reference;
  typedef type_t* pointer;
  typedef const type_t* const_pointer;
  typedef const type_t* const const_pointer_const;

  typedef type_t&& rvalue_reference;
};

template <typename T>
using types_t = typename types<T>::type;

template <typename T>
using types_r = typename types<T>::reference;

template <typename T>
using types_cr = typename types<T>::const_reference;

template <typename T>
using types_rr = typename types<T>::rvalue_reference;

template <typename T>
using types_p = typename types<T>::pointer;

template <typename T>
using types_cp = typename types<T>::const_pointer;

template <typename T>
using types_cpc = typename types<T>::const_pointer_const;

template <typename T>
struct size_of : etl::integral_constant<size_t, sizeof(T)> {};
template <>
struct size_of<void> : etl::integral_constant<size_t, 1U> {};

template <typename T>
inline constexpr size_t size_of_v = etl::size_of<T>::value;

template <typename T, typename T1, typename... TRest>
struct are_all_same {
  static const bool value =
      etl::is_same<T, T1>::value && etl::are_all_same<T, TRest...>::value;
};

template <typename T, typename T1>
struct are_all_same<T, T1> {
  static const bool value = etl::is_same<T, T1>::value;
};

template <typename T, typename T1, typename... TRest>
inline constexpr bool are_all_same_v = are_all_same<T, T1, TRest...>::value;

template <typename...>
struct conjunction : public etl::true_type {};

template <typename T1, typename... Tn>
struct conjunction<T1, Tn...>
    : public etl::conditional_t<bool(T1::value), etl::conjunction<Tn...>, T1> {
};

template <typename T>
struct conjunction<T> : public T {};

template <typename... T>
inline constexpr bool conjunction_v = conjunction<T...>::value;

template <typename...>
struct disjunction : public etl::false_type {};

template <typename T1, typename... Tn>
struct disjunction<T1, Tn...>
    : public etl::conditional_t<bool(T1::value), T1, disjunction<Tn...>> {};

template <typename T1>
struct disjunction<T1> : public T1 {};

template <typename... T>
inline constexpr bool disjunction_v = etl::disjunction<T...>::value;

template <typename T1, typename T2>
struct is_assignable : public std::is_assignable<T1, T2> {};

template <typename T, typename... TArgs>
struct is_constructible : public std::is_constructible<T, TArgs...> {};

template <typename T>
struct is_copy_constructible : public std::is_copy_constructible<T> {};

template <typename T>
struct is_move_constructible : public std::is_move_constructible<T> {};

template <typename T>
struct is_trivially_constructible : public std::is_trivially_constructible<T> {
};

template <typename T>
struct is_trivially_copy_constructible
    : public std::is_trivially_copy_constructible<T> {};

template <typename T>
struct is_trivially_destructible : public std::is_trivially_destructible<T> {};

template <typename T>
struct is_trivially_copy_assignable
    : public std::is_trivially_copy_assignable<T> {};

template <typename T>
struct is_trivially_copyable : public std::is_trivially_copyable<T> {};

template <typename T1, typename T2>
struct is_lvalue_assignable
    : public etl::is_assignable<typename etl::add_lvalue_reference<T1>::type,
                                typename etl::add_lvalue_reference<
                                    typename etl::add_const<T2>::type>::type> {
};

template <typename T1, typename T2>
inline constexpr bool is_assignable_v = etl::is_assignable<T1, T2>::value;

template <typename T1, typename T2>
inline constexpr bool is_lvalue_assignable_v =
    etl::is_lvalue_assignable<T1, T2>::value;

template <typename T, typename... TArgs>
inline constexpr bool is_constructible_v =
    etl::is_constructible<T, TArgs...>::value;

template <typename T>
inline constexpr bool is_copy_constructible_v =
    etl::is_copy_constructible<T>::value;

template <typename T>
inline constexpr bool is_move_constructible_v =
    etl::is_move_constructible<T>::value;

template <typename T>
inline constexpr bool is_trivially_constructible_v =
    etl::is_trivially_constructible<T>::value;

template <typename T>
inline constexpr bool is_trivially_copy_constructible_v =
    etl::is_trivially_copy_constructible<T>::value;

template <typename T>
inline constexpr bool is_trivially_destructible_v =
    etl::is_trivially_destructible<T>::value;

template <typename T>
inline constexpr bool is_trivially_copy_assignable_v =
    etl::is_trivially_copy_assignable<T>::value;

template <typename T>
inline constexpr bool is_trivially_copyable_v =
    etl::is_trivially_copyable<T>::value;

template <typename...>
struct common_type {};

template <typename T>
struct common_type<T> : common_type<T, T> {};

namespace private_common_type {
template <typename T1, typename T2>
using conditional_result_t = decltype(false ? declval<T1>() : declval<T2>());

template <typename, typename, typename = void>
struct decay_conditional_result {};

template <typename T1, typename T2>
struct decay_conditional_result<T1, T2, void_t<conditional_result_t<T1, T2>>>
    : etl::decay<conditional_result_t<T1, T2>> {};

template <typename T1, typename T2, typename = void>
struct common_type_2_impl : decay_conditional_result<const T1&, const T2&> {};

template <typename T1, typename T2>
struct common_type_2_impl<T1, T2, void_t<conditional_result_t<T1, T2>>>
    : decay_conditional_result<T1, T2> {};
}  // namespace private_common_type

template <typename T1, typename T2>
struct common_type<T1, T2>
    : etl::conditional<
          etl::is_same<T1, typename etl::decay<T1>::type>::value &&
              etl::is_same<T2, typename etl::decay<T2>::type>::value,
          private_common_type::common_type_2_impl<T1, T2>,
          common_type<typename etl::decay<T2>::type,
                      typename etl::decay<T2>::type>>::type {};

namespace private_common_type {
template <typename AlwaysVoid, typename T1, typename T2, typename... TRest>
struct common_type_multi_impl {};

template <typename T1, typename T2, typename... TRest>
struct common_type_multi_impl<void_t<typename common_type<T1, T2>::type>,
                              T1,
                              T2,
                              TRest...>
    : common_type<typename common_type<T1, T2>::type, TRest...> {};
}  // namespace private_common_type

template <typename T1, typename T2, typename... TRest>
struct common_type<T1, T2, TRest...>
    : private_common_type::common_type_multi_impl<void, T1, T2, TRest...> {};

template <typename... T>
using common_type_t = typename common_type<T...>::type;

}  // namespace etl

namespace etl {

template <typename T>
constexpr typename etl::remove_reference<T>::type&& move(T&& t) noexcept {
  return static_cast<typename etl::remove_reference<T>::type&&>(t);
}

template <typename T>
constexpr T&& forward(typename etl::remove_reference<T>::type& t) noexcept {
  return static_cast<T&&>(t);
}

template <typename T>
constexpr T&& forward(typename etl::remove_reference<T>::type&& t) noexcept {
  return static_cast<T&&>(t);
}
template <typename T1, typename T2>
struct pair {
  typedef T1 first_type;
  typedef T2 second_type;

  T1 first;
  T2 second;

  constexpr pair() : first(T1()), second(T2()) {}

  constexpr pair(const T1& a, const T2& b) : first(a), second(b) {}

  template <typename U1, typename U2>
  constexpr pair(U1&& a, U2&& b)
      : first(etl::forward<U1>(a)), second(etl::forward<U2>(b)) {}

  template <typename U1, typename U2>
  constexpr pair(const pair<U1, U2>& other)
      : first(other.first), second(other.second) {}

  pair(const pair<T1, T2>& other) : first(other.first), second(other.second) {}

  template <typename U1, typename U2>
  constexpr pair(pair<U1, U2>&& other)
      : first(etl::forward<U1>(other.first)),
        second(etl::forward<U2>(other.second)) {}

  template <typename U1, typename U2>
  operator std::pair<U1, U2>() {
    return std::make_pair(first, second);
  }

  template <typename U1, typename U2>
  pair(const std::pair<U1, U2>& other)
      : first(other.first), second(other.second) {}

  template <typename U1, typename U2>
  pair(std::pair<U1, U2>&& other)
      : first(etl::forward<U1>(other.first)),
        second(etl::forward<U2>(other.second)) {}

  void swap(pair<T1, T2>& other) {
    using std::swap;

    swap(first, other.first);
    swap(second, other.second);
  }

  pair<T1, T2>& operator=(const pair<T1, T2>& other) {
    first = other.first;
    second = other.second;

    return *this;
  }

  template <typename U1, typename U2>
  pair<U1, U2>& operator=(const pair<U1, U2>& other) {
    first = other.first;
    second = other.second;

    return *this;
  }

  pair<T1, T2>& operator=(pair<T1, T2>&& other) {
    first = etl::forward<T1>(other.first);
    second = etl::forward<T2>(other.second);

    return *this;
  }

  template <typename U1, typename U2>
  pair<U1, U2>& operator=(pair<U1, U2>&& other) {
    first = etl::forward<U1>(other.first);
    second = etl::forward<U2>(other.second);

    return *this;
  }
};

template <typename T1, typename T2>
inline pair<T1, T2> make_pair(T1&& a, T2&& b) {
  return pair<T1, T2>(etl::forward<T1>(a), etl::forward<T2>(b));
}
template <typename T1, typename T2>
inline void swap(pair<T1, T2>& a, pair<T1, T2>& b) {
  a.swap(b);
}

template <typename T1, typename T2>
inline bool operator==(const pair<T1, T2>& a, const pair<T1, T2>& b) {
  return (a.first == b.first) && (a.second == b.second);
}

template <typename T1, typename T2>
inline bool operator!=(const pair<T1, T2>& a, const pair<T1, T2>& b) {
  return !(a == b);
}

template <typename T1, typename T2>
inline bool operator<(const pair<T1, T2>& a, const pair<T1, T2>& b) {
  return (a.first < b.first) || (!(b.first < a.first) && (a.second < b.second));
}

template <typename T1, typename T2>
inline bool operator>(const pair<T1, T2>& a, const pair<T1, T2>& b) {
  return (b < a);
}

template <typename T1, typename T2>
inline bool operator<=(const pair<T1, T2>& a, const pair<T1, T2>& b) {
  return !(b < a);
}

template <typename T1, typename T2>
inline bool operator>=(const pair<T1, T2>& a, const pair<T1, T2>& b) {
  return !(a < b);
}

template <typename T>
T exchange(T& object, const T& new_value) {
  T old_value = object;
  object = new_value;
  return old_value;
}

template <typename T, typename U>
T exchange(T& object, const U& new_value) {
  T old_value = object;
  object = new_value;
  return old_value;
}

template <typename T>
typename etl::add_const<T>::type& as_const(T& t) {
  return t;
}

template <typename T, T... Integers>
class integer_sequence {
 public:
  static_assert(etl::is_integral<T>::value, "Integral types only");

  typedef T value_type;

  static constexpr size_t size() noexcept { return sizeof...(Integers); }
};

namespace private_integer_sequence {
template <size_t N, typename IndexSeq>
struct make_index_sequence;

template <size_t N, size_t... Indices>
struct make_index_sequence<N, etl::integer_sequence<size_t, Indices...>> {
  typedef typename make_index_sequence<
      N - 1,
      etl::integer_sequence<size_t, N - 1, Indices...>>::type type;
};

template <size_t... Indices>
struct make_index_sequence<0, etl::integer_sequence<size_t, Indices...>> {
  typedef etl::integer_sequence<size_t, Indices...> type;
};
}  // namespace private_integer_sequence

template <size_t N>
using make_index_sequence = typename private_integer_sequence::
    make_index_sequence<N, etl::integer_sequence<size_t>>::type;

template <size_t... Indices>
using index_sequence = etl::integer_sequence<size_t, Indices...>;

template <typename T>
struct coordinate_2d {
  coordinate_2d() : x(T(0)), y(T(0)) {}

  coordinate_2d(T x_, T y_) : x(x_), y(y_) {}

  friend bool operator==(const coordinate_2d& lhs, const coordinate_2d& rhs) {
    return (lhs.x == rhs.x) && (lhs.y == rhs.y);
  }

  friend bool operator!=(const coordinate_2d& lhs, const coordinate_2d& rhs) {
    return !(lhs == rhs);
  }

  T x;
  T y;
};

struct in_place_t {
  explicit constexpr in_place_t() {}
};

inline constexpr in_place_t in_place{};

template <typename T>
struct in_place_type_t {
  explicit constexpr in_place_type_t() {}
};

template <typename T>
inline constexpr in_place_type_t<T> in_place_type{};

template <size_t I>
struct in_place_index_t {
  explicit constexpr in_place_index_t() {}
};

template <size_t I>
inline constexpr in_place_index_t<I> in_place_index{};

template <typename T>
typename etl::add_rvalue_reference<T>::type declval() noexcept;

template <typename TReturn, typename... TParams>
class functor {
 public:
  constexpr functor(TReturn (*ptr_)(TParams...)) : ptr(ptr_) {}

  constexpr TReturn operator()(TParams... args) const {
    return ptr(etl::forward<TParams>(args)...);
  }

 private:
  TReturn (*ptr)(TParams...);
};

template <typename T>
class member_function_wrapper;

template <typename TReturn, typename... TParams>
class member_function_wrapper<TReturn(TParams...)> {
 public:
  template <typename T, T& Instance, TReturn (T::*Method)(TParams...)>
  static constexpr TReturn function(TParams... params) {
    return (Instance.*Method)(etl::forward<TParams>(params)...);
  }
};

template <typename T>
class functor_wrapper;

template <typename TReturn, typename... TParams>
class functor_wrapper<TReturn(TParams...)> {
 public:
  template <typename TFunctor, TFunctor& Instance>
  static constexpr TReturn function(TParams... params) {
    return Instance(etl::forward<TParams>(params)...);
  }
};

}  // namespace etl

namespace etl {

struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag {};
struct bidirectional_iterator_tag : public forward_iterator_tag {};
struct random_access_iterator_tag : public bidirectional_iterator_tag {};
struct contiguous_iterator_tag : public random_access_iterator_tag {};

template <typename TIterator,
          typename = typename etl::
              enable_if<!etl::is_fundamental<TIterator>::value, void>::type>
struct iterator_traits {
  typedef typename TIterator::iterator_category iterator_category;
  typedef typename TIterator::value_type value_type;
  typedef typename TIterator::difference_type difference_type;
  typedef typename TIterator::pointer pointer;
  typedef typename TIterator::reference reference;
};

template <typename T>
struct iterator_traits<T*, void> {
  typedef std::random_access_iterator_tag iterator_category;
  typedef T value_type;
  typedef ptrdiff_t difference_type;
  typedef typename etl::remove_cv<T>::type* pointer;
  typedef T& reference;
};

template <typename T>
struct iterator_traits<const T*, void> {
  typedef std::random_access_iterator_tag iterator_category;
  typedef T value_type;
  typedef ptrdiff_t difference_type;
  typedef const typename etl::remove_cv<T>::type* pointer;
  typedef const T& reference;
};
template <typename TIterator, typename TDistance>
constexpr void advance(TIterator& itr, TDistance n) {
  std::advance(itr, n);
}
template <typename TIterator>
constexpr typename std::iterator_traits<TIterator>::difference_type distance(
    TIterator first, TIterator last) {
  return std::distance(first, last);
}

template <typename TIterator>
constexpr TIterator prev(
    TIterator itr,
    typename etl::iterator_traits<TIterator>::difference_type n = 1) {
  std::advance(itr, -n);

  return itr;
}

template <typename TIterator>
constexpr TIterator next(
    TIterator itr,
    typename etl::iterator_traits<TIterator>::difference_type n = 1) {
  std::advance(itr, n);

  return itr;
}

template <typename TIterator>
class reverse_iterator {
 public:
  typedef
      typename iterator_traits<TIterator>::iterator_category iterator_category;
  typedef typename iterator_traits<TIterator>::value_type value_type;
  typedef typename iterator_traits<TIterator>::difference_type difference_type;
  typedef typename iterator_traits<TIterator>::pointer pointer;
  typedef typename iterator_traits<TIterator>::reference reference;

  typedef TIterator iterator_type;

  constexpr reverse_iterator() : current() {}

  constexpr explicit reverse_iterator(TIterator itr) : current(itr) {}

  template <typename TOther>
  constexpr reverse_iterator(const reverse_iterator<TOther>& other)
      : current(other.base()) {}

  template <class TOther>
  constexpr reverse_iterator& operator=(const reverse_iterator<TOther>& other) {
    current = other.base();

    return (*this);
  }

  constexpr TIterator base() const { return current; }

  [[nodiscard]] constexpr reference operator*() const {
    TIterator temp = current;

    return *(--temp);
  }

  [[nodiscard]] constexpr pointer operator->() const {
    TIterator temp = current;

    return &(*--temp);
  }

  constexpr reverse_iterator& operator++() {
    --current;

    return *this;
  }

  constexpr reverse_iterator operator++(int) {
    reverse_iterator temp = *this;
    --current;

    return temp;
  }

  constexpr reverse_iterator& operator--() {
    ++current;

    return (*this);
  }

  constexpr reverse_iterator operator--(int) {
    reverse_iterator temp = *this;
    ++current;

    return temp;
  }

  constexpr reverse_iterator& operator+=(const difference_type offset) {
    current -= offset;

    return (*this);
  }

  constexpr reverse_iterator& operator-=(const difference_type offset) {
    current += offset;

    return (*this);
  }

  [[nodiscard]] constexpr reverse_iterator operator+(
      const difference_type offset) const {
    return reverse_iterator(current - offset);
  }

  [[nodiscard]] constexpr reverse_iterator operator-(
      const difference_type offset) const {
    return (reverse_iterator(current + offset));
  }

  [[nodiscard]] constexpr reference operator[](
      const difference_type offset) const {
    return (*(*this + offset));
  }

 protected:
  TIterator current;
};

template <class TIterator>
constexpr bool operator==(const reverse_iterator<TIterator>& lhs,
                          const reverse_iterator<TIterator>& rhs) {
  return lhs.base() == rhs.base();
}

template <class TIterator>
constexpr bool operator!=(const reverse_iterator<TIterator>& lhs,
                          const reverse_iterator<TIterator>& rhs) {
  return !(lhs == rhs);
}

template <class TIterator>
constexpr bool operator<(const reverse_iterator<TIterator>& lhs,
                         const reverse_iterator<TIterator>& rhs) {
  return rhs.base() < lhs.base();
}

template <class TIterator>
constexpr bool operator>(const reverse_iterator<TIterator>& lhs,
                         const reverse_iterator<TIterator>& rhs) {
  return rhs < lhs;
}

template <class TIterator>
constexpr bool operator<=(const reverse_iterator<TIterator>& lhs,
                          const reverse_iterator<TIterator>& rhs) {
  return !(rhs < lhs);
}

template <class TIterator>
constexpr bool operator>=(const reverse_iterator<TIterator>& lhs,
                          const reverse_iterator<TIterator>& rhs) {
  return !(lhs < rhs);
}

template <class TIterator>
constexpr typename reverse_iterator<TIterator>::difference_type operator-(
    const reverse_iterator<TIterator>& lhs,
    const reverse_iterator<TIterator>& rhs) {
  return rhs.base() - lhs.base();
}

template <class TIterator, class TDifference>
constexpr reverse_iterator<TIterator> operator+(
    TDifference n, const reverse_iterator<TIterator>& itr) {
  return itr.operator+(n);
}

template <typename TCategory,
          typename T,
          typename TDistance = ptrdiff_t,
          typename TPointer = T*,
          typename TReference = T&>
struct iterator {
  typedef T value_type;
  typedef TDistance difference_type;
  typedef TPointer pointer;
  typedef TReference reference;
  typedef TCategory iterator_category;
};

template <typename TIterator>
class move_iterator {
 public:
  typedef
      typename iterator_traits<TIterator>::iterator_category iterator_category;
  typedef typename iterator_traits<TIterator>::value_type value_type;
  typedef typename iterator_traits<TIterator>::difference_type difference_type;
  typedef TIterator iterator_type;
  typedef TIterator pointer;
  typedef value_type&& reference;

  move_iterator() {}

  explicit move_iterator(TIterator itr) : current(itr) {}

  template <typename U>
  move_iterator(const move_iterator<U>& itr) : current(itr.base()) {}

  template <typename U>
  move_iterator& operator=(const move_iterator<U>& itr) {
    current = itr.current;
    return *this;
  }

  iterator_type base() const { return current; }

  pointer operator->() const { return current; }

  reference operator*() const { return etl::move(*current); }

  move_iterator& operator++() {
    ++current;
    return *this;
  }

  move_iterator& operator--() {
    --current;
    return *this;
  }

  move_iterator operator++(int) {
    move_iterator temp = *this;
    ++current;
    return temp;
  }

  move_iterator operator--(int) {
    move_iterator temp = *this;
    --current;
    return temp;
  }

  move_iterator operator+(difference_type n) const {
    return move_iterator(current + n);
  }

  move_iterator operator-(difference_type n) const {
    return move_iterator(current - n);
  }

  move_iterator operator+=(difference_type n) {
    current += n;
    return *this;
  }

  move_iterator operator-=(difference_type n) {
    current -= n;
    return *this;
  }

  reference operator[](difference_type n) const {
    return etl::move(current[n]);
  }

 private:
  TIterator current;
};

template <typename TIterator>
bool operator==(const etl::move_iterator<TIterator>& lhs,
                const etl::move_iterator<TIterator>& rhs) {
  return lhs.base() == rhs.base();
}

template <typename TIterator>
bool operator!=(const etl::move_iterator<TIterator>& lhs,
                const etl::move_iterator<TIterator>& rhs) {
  return !(lhs == rhs);
}

template <typename TIterator>
bool operator<(const etl::move_iterator<TIterator>& lhs,
               const etl::move_iterator<TIterator>& rhs) {
  return lhs.base() < rhs.base();
}

template <typename TIterator>
bool operator<=(const etl::move_iterator<TIterator>& lhs,
                const etl::move_iterator<TIterator>& rhs) {
  return !(rhs < lhs);
}

template <typename TIterator>
bool operator>(const etl::move_iterator<TIterator>& lhs,
               const etl::move_iterator<TIterator>& rhs) {
  return (rhs < lhs);
}

template <typename TIterator>
bool operator>=(const etl::move_iterator<TIterator>& lhs,
                const etl::move_iterator<TIterator>& rhs) {
  return !(lhs < rhs);
}

template <typename TIterator>
move_iterator<TIterator> operator+(
    typename move_iterator<TIterator>::difference_type n,
    const move_iterator<TIterator>& rhs) {
  return rhs + n;
}

template <typename TIterator1, typename TIterator2>
auto operator-(const move_iterator<TIterator1>& lhs,
               const move_iterator<TIterator2>& rhs)
    -> decltype(lhs.base() - rhs.base()) {
  return lhs.base() - rhs.base();
}

template <typename TIterator>
etl::move_iterator<TIterator> make_move_iterator(TIterator itr) {
  return etl::move_iterator<TIterator>(itr);
}

template <typename T>
struct is_input_iterator {
  static constexpr bool value =
      etl::is_same<typename etl::iterator_traits<T>::iterator_category,
                   std::input_iterator_tag>::value;
};

template <typename T>
struct is_output_iterator {
  static constexpr bool value =
      etl::is_same<typename etl::iterator_traits<T>::iterator_category,
                   std::output_iterator_tag>::value;
};

template <typename T>
struct is_forward_iterator {
  static constexpr bool value =
      etl::is_same<typename etl::iterator_traits<T>::iterator_category,
                   std::forward_iterator_tag>::value;
};

template <typename T>
struct is_bidirectional_iterator {
  static constexpr bool value =
      etl::is_same<typename etl::iterator_traits<T>::iterator_category,
                   std::bidirectional_iterator_tag>::value;
};

template <typename T>
struct is_random_iterator {
  static constexpr bool value =
      etl::is_same<typename etl::iterator_traits<T>::iterator_category,
                   std::random_access_iterator_tag>::value;
};

template <typename T>
struct is_random_access_iterator {
  static constexpr bool value =
      etl::is_same<typename etl::iterator_traits<T>::iterator_category,
                   std::random_access_iterator_tag>::value;
};

template <typename T>
struct is_input_iterator_concept {
  static constexpr bool value = etl::is_input_iterator<T>::value ||
                                etl::is_forward_iterator<T>::value ||
                                etl::is_bidirectional_iterator<T>::value ||
                                etl::is_random_iterator<T>::value;
};

template <typename T>
struct is_output_iterator_concept {
  static constexpr bool value = etl::is_output_iterator<T>::value ||
                                etl::is_forward_iterator<T>::value ||
                                etl::is_bidirectional_iterator<T>::value ||
                                etl::is_random_iterator<T>::value;
};

template <typename T>
struct is_forward_iterator_concept {
  static constexpr bool value = etl::is_forward_iterator<T>::value ||
                                etl::is_bidirectional_iterator<T>::value ||
                                etl::is_random_iterator<T>::value;
};

template <typename T>
struct is_bidirectional_iterator_concept {
  static constexpr bool value = etl::is_bidirectional_iterator<T>::value ||
                                etl::is_random_iterator<T>::value;
};

template <typename T>
struct is_random_iterator_concept {
  static constexpr bool value = etl::is_random_iterator<T>::value;
};

template <typename T>
struct is_random_access_iterator_concept {
  static constexpr bool value = etl::is_random_access_iterator<T>::value;
};
template <typename T, const size_t ARRAY_SIZE>
char (&array_size(T (&array)[ARRAY_SIZE]))[ARRAY_SIZE];
}  // namespace etl

namespace etl {

template <typename T>
class reference_wrapper {
 public:
  typedef T type;

  explicit reference_wrapper(T& t_) noexcept : t(&t_) {}

  reference_wrapper<T>& operator=(const reference_wrapper& rhs) noexcept {
    t = rhs.t;
    return *this;
  }

  T& get() const noexcept { return *t; }

  operator T&() const noexcept { return *t; }

 private:
  T* t;
};

template <typename T>
reference_wrapper<T> ref(T& t) {
  return reference_wrapper<T>(t);
}

template <typename T>
reference_wrapper<T> ref(reference_wrapper<T> t) {
  return reference_wrapper<T>(t.get());
}

template <typename T>
reference_wrapper<const T> cref(const T& t) {
  return reference_wrapper<const T>(t);
}

template <typename T>
reference_wrapper<const T> cref(reference_wrapper<T> t) {
  return reference_wrapper<const T>(t.get());
}

template <typename TArgumentType, typename TResultType>
struct unary_function {
  typedef TArgumentType argument_type;
  typedef TResultType result_type;
};

template <typename TFirstArgumentType,
          typename TSecondArgumentType,
          typename TResultType>
struct binary_function {
  typedef TFirstArgumentType first_argument_type;
  typedef TSecondArgumentType second_argument_type;
  typedef TResultType result_type;
};

template <typename T = void>
struct less : public etl::binary_function<T, T, bool> {
  typedef T value_type;

  constexpr bool operator()(const T& lhs, const T& rhs) const {
    return (lhs < rhs);
  }
};

template <>
struct less<void> : public etl::binary_function<void, void, bool> {
  typedef int is_transparent;

  template <typename T1, typename T2>
  constexpr auto operator()(T1&& lhs, T2&& rhs) const
      -> decltype(static_cast<T1&&>(lhs) < static_cast<T2&&>(rhs)) {
    return static_cast<T1&&>(lhs) < static_cast<T2&&>(rhs);
  }
};

template <typename T = void>
struct less_equal : public etl::binary_function<T, T, bool> {
  typedef T value_type;

  constexpr bool operator()(const T& lhs, const T& rhs) const {
    return !(rhs < lhs);
  }
};

template <>
struct less_equal<void> : public etl::binary_function<void, void, bool> {
  typedef int is_transparent;

  template <typename T1, typename T2>
  constexpr auto operator()(T1&& lhs, T2&& rhs) const
      -> decltype(static_cast<T1&&>(lhs) < static_cast<T2&&>(rhs)) {
    return !(static_cast<T1&&>(lhs) < static_cast<T2&&>(rhs));
  }
};

template <typename T = void>
struct greater : public etl::binary_function<T, T, bool> {
  typedef T value_type;

  constexpr bool operator()(const T& lhs, const T& rhs) const {
    return (rhs < lhs);
  }
};

template <>
struct greater<void> : public etl::binary_function<void, void, bool> {
  typedef int is_transparent;

  template <typename T1, typename T2>
  constexpr auto operator()(T1&& lhs, T2&& rhs) const
      -> decltype(static_cast<T1&&>(lhs) < static_cast<T2&&>(rhs)) {
    return static_cast<T1&&>(rhs) < static_cast<T2&&>(lhs);
  }
};

template <typename T = void>
struct greater_equal : public etl::binary_function<T, T, bool> {
  typedef T value_type;

  constexpr bool operator()(const T& lhs, const T& rhs) const {
    return !(lhs < rhs);
  }
};

template <>
struct greater_equal<void> : public etl::binary_function<void, void, bool> {
  typedef int is_transparent;

  template <typename T1, typename T2>
  constexpr auto operator()(T1&& lhs, T2&& rhs) const
      -> decltype(static_cast<T1&&>(lhs) < static_cast<T2&&>(rhs)) {
    return static_cast<T1&&>(rhs) < static_cast<T2&&>(lhs);
  }
};

template <typename T = void>
struct equal_to : public etl::binary_function<T, T, bool> {
  typedef T value_type;

  constexpr bool operator()(const T& lhs, const T& rhs) const {
    return lhs == rhs;
  }
};

template <>
struct equal_to<void> : public etl::binary_function<void, void, bool> {
  typedef void value_type;
  typedef int is_transparent;

  template <typename T1, typename T2>
  constexpr auto operator()(T1&& lhs, T2&& rhs) const
      -> decltype(static_cast<T1&&>(lhs) < static_cast<T2&&>(rhs)) {
    return static_cast<T1&&>(lhs) == static_cast<T2&&>(rhs);
  }
};

template <typename T = void>
struct not_equal_to : public etl::binary_function<T, T, bool> {
  typedef T value_type;

  constexpr bool operator()(const T& lhs, const T& rhs) const {
    return !(lhs == rhs);
  }
};

template <>
struct not_equal_to<void> : public etl::binary_function<void, void, bool> {
  typedef int is_transparent;

  template <typename T1, typename T2>
  constexpr auto operator()(T1&& lhs, T2&& rhs) const
      -> decltype(static_cast<T1&&>(lhs) < static_cast<T2&&>(rhs)) {
    return !(static_cast<T1&&>(lhs) == static_cast<T2&&>(rhs));
  }
};

template <typename TFunction>
class binder1st
    : public etl::unary_function<typename TFunction::second_argument_type,
                                 typename TFunction::result_type> {
 protected:
  TFunction operation;
  typename TFunction::first_argument_type value;

 public:
  binder1st(const TFunction& f,
            const typename TFunction::first_argument_type& v)
      : operation(f), value(v) {}

  typename TFunction::result_type operator()(
      typename TFunction::second_argument_type& x) const {
    return operation(value, x);
  }

  typename TFunction::result_type operator()(
      const typename TFunction::second_argument_type& x) const {
    return operation(value, x);
  }
};

template <typename F, typename T>
binder1st<F> bind1st(const F& f, const T& x) {
  return binder1st<F>(f, x);
}

template <typename TFunction>
class binder2nd
    : public etl::unary_function<typename TFunction::first_argument_type,
                                 typename TFunction::result_type> {
 protected:
  TFunction operation;
  typename TFunction::second_argument_type value;

 public:
  binder2nd(const TFunction& f,
            const typename TFunction::second_argument_type& v)
      : operation(f), value(v) {}

  typename TFunction::result_type operator()(
      typename TFunction::first_argument_type& x) const {
    return operation(x, value);
  }

  typename TFunction::result_type operator()(
      const typename TFunction::first_argument_type& x) const {
    return operation(x, value);
  }
};

template <typename F, typename T>
binder2nd<F> bind2nd(const F& f, const T& x) {
  return binder2nd<F>(f, x);
}

template <typename T = void>
struct plus {
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;

  constexpr T operator()(const T& lhs, const T& rhs) const { return lhs + rhs; }
};

template <typename T = void>
struct minus {
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;

  constexpr T operator()(const T& lhs, const T& rhs) const { return lhs - rhs; }
};

template <typename T = void>
struct negate {
  typedef T argument_type;
  typedef T result_type;

  constexpr T operator()(const T& lhs) const { return -lhs; }
};

template <typename T = void>
struct multiplies {
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;

  constexpr T operator()(const T& lhs, const T& rhs) const { return lhs * rhs; }
};

template <typename T = void>
struct divides {
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;

  constexpr T operator()(const T& lhs, const T& rhs) const { return lhs / rhs; }
};

template <typename T = void>
struct modulus {
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;

  constexpr T operator()(const T& lhs, const T& rhs) const { return lhs % rhs; }
};

template <typename T = void>
struct logical_and {
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;

  constexpr T operator()(const T& lhs, const T& rhs) const {
    return lhs && rhs;
  }
};

template <typename T = void>
struct logical_or {
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;

  constexpr T operator()(const T& lhs, const T& rhs) const {
    return lhs || rhs;
  }
};

template <typename T = void>
struct logical_not {
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;

  constexpr T operator()(const T& lhs) const { return !lhs; }
};

template <typename T = void>
struct bit_and {
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;

  constexpr T operator()(const T& lhs, const T& rhs) const { return lhs & rhs; }
};

template <typename T = void>
struct bit_or {
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;

  constexpr T operator()(const T& lhs, const T& rhs) const { return lhs | rhs; }
};

template <typename T = void>
struct bit_xor {
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;

  constexpr T operator()(const T& lhs, const T& rhs) const { return lhs ^ rhs; }
};

template <typename T = void>
struct bit_not {
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;

  constexpr T operator()(const T& lhs) const { return ~lhs; }
};
}  // namespace etl

namespace etl {

template <typename TIterator>

void shell_sort(TIterator first, TIterator last);

template <typename TIterator, typename TCompare>

void shell_sort(TIterator first, TIterator last, TCompare compare);

template <typename TIterator>
constexpr void insertion_sort(TIterator first, TIterator last);

template <typename TIterator, typename TCompare>
constexpr void insertion_sort(TIterator first,
                              TIterator last,
                              TCompare compare);
}  // namespace etl

namespace etl {

template <typename TIterator1, typename TIterator2>

void iter_swap(TIterator1 a, TIterator2 b) {
  using std::swap;
  swap(*a, *b);
}

template <typename T1terator1, typename TIterator2>

TIterator2 swap_ranges(T1terator1 first1, T1terator1 last1, TIterator2 first2) {
  while (first1 != last1) {
    iter_swap(first1, first2);
    ++first1;
    ++first2;
  }

  return first2;
}
template <typename TIterator1, typename TIterator2>
constexpr TIterator2 copy(TIterator1 sb, TIterator1 se, TIterator2 db) {
  while (sb != se) {
    *db = *sb;
    ++db;
    ++sb;
  }

  return db;
}
template <typename TIterator1, typename TIterator2>
constexpr TIterator2 reverse_copy(TIterator1 sb, TIterator1 se, TIterator2 db) {
  while (sb != se) {
    *db = *--se;
    ++db;
  }

  return db;
}
template <typename TIterator1, typename TSize, typename TIterator2>
TIterator2 copy_n(TIterator1 sb, TSize count, TIterator2 db) {
  return std::copy_n(sb, count, db);
}
template <typename TIterator1, typename TIterator2>
constexpr TIterator2 copy_backward(TIterator1 sb,
                                   TIterator1 se,
                                   TIterator2 de) {
  while (se != sb) {
    *(--de) = *(--se);
  }

  return de;
}
template <typename TIterator1, typename TIterator2>
TIterator2 move(TIterator1 sb, TIterator1 se, TIterator2 db) {
  return std::move(sb, se, db);
}
template <typename TIterator1, typename TIterator2>
TIterator2 move_backward(TIterator1 sb, TIterator1 se, TIterator2 de) {
  return std::move_backward(sb, se, de);
}
template <typename TIterator>
typename etl::enable_if<etl::is_pointer<TIterator>::value, void>::type reverse(
    TIterator b, TIterator e) {
  if (b != e) {
    while (b < --e) {
      etl::iter_swap(b, e);
      ++b;
    }
  }
}

template <typename TIterator>
typename etl::enable_if<!etl::is_pointer<TIterator>::value, void>::type reverse(
    TIterator b, TIterator e) {
  while ((b != e) && (b != --e)) {
    etl::iter_swap(b++, e);
  }
}

template <typename TIterator, typename TValue, typename TCompare>
[[nodiscard]] constexpr TIterator lower_bound(TIterator first,
                                              TIterator last,
                                              const TValue& value,
                                              TCompare compare) {
  typedef
      typename etl::iterator_traits<TIterator>::difference_type difference_t;

  difference_t count = etl::distance(first, last);

  while (count > 0) {
    TIterator itr = first;
    difference_t step = count / 2;

    etl::advance(itr, step);

    if (compare(*itr, value)) {
      first = ++itr;
      count -= step + 1;
    } else {
      count = step;
    }
  }

  return first;
}

template <typename TIterator, typename TValue>
[[nodiscard]] constexpr TIterator lower_bound(TIterator first,
                                              TIterator last,
                                              const TValue& value) {
  typedef etl::less<typename etl::iterator_traits<TIterator>::value_type>
      compare;

  return etl::lower_bound(first, last, value, compare());
}

template <typename TIterator, typename TValue, typename TCompare>
[[nodiscard]] constexpr TIterator upper_bound(TIterator first,
                                              TIterator last,
                                              const TValue& value,
                                              TCompare compare) {
  typedef
      typename etl::iterator_traits<TIterator>::difference_type difference_t;

  difference_t count = etl::distance(first, last);

  while (count > 0) {
    TIterator itr = first;
    difference_t step = count / 2;

    etl::advance(itr, step);

    if (!compare(value, *itr)) {
      first = ++itr;
      count -= step + 1;
    } else {
      count = step;
    }
  }

  return first;
}

template <typename TIterator, typename TValue>
[[nodiscard]] constexpr TIterator upper_bound(TIterator first,
                                              TIterator last,
                                              const TValue& value) {
  typedef etl::less<typename etl::iterator_traits<TIterator>::value_type>
      compare;

  return etl::upper_bound(first, last, value, compare());
}

template <typename TIterator, typename TValue, typename TCompare>
[[nodiscard]] constexpr std::pair<TIterator, TIterator> equal_range(
    TIterator first, TIterator last, const TValue& value, TCompare compare) {
  return std::make_pair(etl::lower_bound(first, last, value, compare),
                        etl::upper_bound(first, last, value, compare));
}

template <typename TIterator, typename TValue>
[[nodiscard]] std::pair<TIterator, TIterator> equal_range(TIterator first,
                                                          TIterator last,
                                                          const TValue& value) {
  typedef etl::less<typename etl::iterator_traits<TIterator>::value_type>
      compare;

  return std::make_pair(etl::lower_bound(first, last, value, compare()),
                        etl::upper_bound(first, last, value, compare()));
}

template <typename TIterator, typename TUnaryPredicate>
[[nodiscard]] constexpr TIterator find_if(TIterator first,
                                          TIterator last,
                                          TUnaryPredicate predicate) {
  while (first != last) {
    if (predicate(*first)) {
      return first;
    }

    ++first;
  }

  return last;
}

template <typename TIterator, typename T>
[[nodiscard]] constexpr TIterator find(TIterator first,
                                       TIterator last,
                                       const T& value) {
  while (first != last) {
    if (*first == value) {
      return first;
    }

    ++first;
  }

  return last;
}
template <typename TIterator, typename TValue>
constexpr void fill(TIterator first, TIterator last, const TValue& value) {
  while (first != last) {
    *first = value;
    ++first;
  }
}
template <typename TIterator, typename TSize, typename TValue>
constexpr TIterator fill_n(TIterator first, TSize count, const TValue& value) {
  while (count != 0) {
    *first++ = value;
    --count;
  }

  return first;
}

template <typename TIterator, typename T>
[[nodiscard]] constexpr
    typename etl::iterator_traits<TIterator>::difference_type
    count(TIterator first, TIterator last, const T& value) {
  typename iterator_traits<TIterator>::difference_type n = 0;

  while (first != last) {
    if (*first == value) {
      ++n;
    }

    ++first;
  }

  return n;
}

template <typename TIterator, typename TUnaryPredicate>
[[nodiscard]] constexpr
    typename etl::iterator_traits<TIterator>::difference_type
    count_if(TIterator first, TIterator last, TUnaryPredicate predicate) {
  typename iterator_traits<TIterator>::difference_type n = 0;

  while (first != last) {
    if (predicate(*first)) {
      ++n;
    }

    ++first;
  }

  return n;
}
template <typename TIterator1, typename TIterator2>
[[nodiscard]] constexpr bool equal(TIterator1 first1,
                                   TIterator1 last1,
                                   TIterator2 first2) {
  while (first1 != last1) {
    if (*first1 != *first2) {
      return false;
    }

    ++first1;
    ++first2;
  }

  return true;
}

template <typename TIterator1, typename TIterator2, typename TPredicate>
[[nodiscard]] constexpr bool equal(TIterator1 first1,
                                   TIterator1 last1,
                                   TIterator2 first2,
                                   TPredicate predicate) {
  while (first1 != last1) {
    if (!predicate(*first1, *first2)) {
      return false;
    }

    ++first1;
    ++first2;
  }

  return true;
}

template <typename TIterator1, typename TIterator2>
[[nodiscard]] constexpr bool equal(TIterator1 first1,
                                   TIterator1 last1,
                                   TIterator2 first2,
                                   TIterator2 last2) {
  while ((first1 != last1) && (first2 != last2)) {
    if (*first1 != *first2) {
      return false;
    }

    ++first1;
    ++first2;
  }

  return (first1 == last1) && (first2 == last2);
}

template <typename TIterator1, typename TIterator2, typename TPredicate>
[[nodiscard]] constexpr bool equal(TIterator1 first1,
                                   TIterator1 last1,
                                   TIterator2 first2,
                                   TIterator2 last2,
                                   TPredicate predicate) {
  while ((first1 != last1) && (first2 != last2)) {
    if (!predicate(*first1, *first2)) {
      return false;
    }

    ++first1;
    ++first2;
  }

  return (first1 == last1) && (first2 == last2);
}

template <typename TIterator1, typename TIterator2, typename TCompare>
[[nodiscard]] constexpr bool lexicographical_compare(TIterator1 first1,
                                                     TIterator1 last1,
                                                     TIterator2 first2,
                                                     TIterator2 last2,
                                                     TCompare compare) {
  while ((first1 != last1) && (first2 != last2)) {
    if (compare(*first1, *first2)) {
      return true;
    }

    if (compare(*first2, *first1)) {
      return false;
    }

    ++first1;
    ++first2;
  }

  return (first1 == last1) && (first2 != last2);
}

template <typename TIterator1, typename TIterator2>
[[nodiscard]] constexpr bool lexicographical_compare(TIterator1 first1,
                                                     TIterator1 last1,
                                                     TIterator2 first2,
                                                     TIterator2 last2) {
  typedef etl::less<typename etl::iterator_traits<TIterator1>::value_type>
      compare;

  return etl::lexicographical_compare(first1, last1, first2, last2, compare());
}

template <typename T, typename TCompare>
[[nodiscard]] constexpr const T& min(const T& a, const T& b, TCompare compare) {
  return (compare(a, b)) ? a : b;
}

template <typename T>
[[nodiscard]] constexpr const T& min(const T& a, const T& b) {
  typedef etl::less<T> compare;

  return etl::min(a, b, compare());
}

template <typename T, typename TCompare>
[[nodiscard]] constexpr const T& max(const T& a, const T& b, TCompare compare) {
  return (compare(a, b)) ? b : a;
}

template <typename T>
[[nodiscard]] constexpr const T& max(const T& a, const T& b) {
  typedef etl::less<T> compare;

  return etl::max(a, b, compare());
}

template <typename TIterator, typename TUnaryOperation>
constexpr TUnaryOperation for_each(TIterator first,
                                   TIterator last,
                                   TUnaryOperation unary_operation) {
  while (first != last) {
    unary_operation(*first);
    ++first;
  }

  return unary_operation;
}

template <typename TIteratorIn, typename TIteratorOut, typename TUnaryOperation>
constexpr TIteratorOut transform(TIteratorIn first1,
                                 TIteratorIn last1,
                                 TIteratorOut d_first,
                                 TUnaryOperation unary_operation) {
  while (first1 != last1) {
    *d_first = unary_operation(*first1);

    ++d_first;
    ++first1;
  }

  return d_first;
}

template <typename TIteratorIn1,
          typename TIteratorIn2,
          typename TIteratorOut,
          typename TBinaryOperation>
constexpr TIteratorOut transform(TIteratorIn1 first1,
                                 TIteratorIn1 last1,
                                 TIteratorIn2 first2,
                                 TIteratorOut d_first,
                                 TBinaryOperation binary_operation) {
  while (first1 != last1) {
    *d_first = binary_operation(*first1, *first2);

    ++d_first;
    ++first1;
    ++first2;
  }

  return d_first;
}

template <typename TIterator, typename T>
constexpr void replace(TIterator first,
                       TIterator last,
                       const T& old_value,
                       const T& new_value) {
  while (first != last) {
    if (*first == old_value) {
      *first = new_value;
    }

    ++first;
  }
}

template <typename TIterator, typename TPredicate, typename T>
constexpr void replace_if(TIterator first,
                          TIterator last,
                          TPredicate predicate,
                          const T& new_value) {
  while (first != last) {
    if (predicate(*first)) {
      *first = new_value;
    }

    ++first;
  }
}

namespace private_heap {

template <typename TIterator,
          typename TDistance,
          typename TValue,
          typename TCompare>
void push_heap(TIterator first,
               TDistance value_index,
               TDistance top_index,
               TValue value,
               TCompare compare) {
  TDistance parent = (value_index - 1) / 2;

  while ((value_index > top_index) && compare(first[parent], value)) {
    first[value_index] = etl::move(first[parent]);
    value_index = parent;
    parent = (value_index - 1) / 2;
  }

  first[value_index] = etl::move(value);
}

template <typename TIterator,
          typename TDistance,
          typename TValue,
          typename TCompare>
void adjust_heap(TIterator first,
                 TDistance value_index,
                 TDistance length,
                 TValue value,
                 TCompare compare) {
  TDistance top_index = value_index;
  TDistance child2nd = (2 * value_index) + 2;

  while (child2nd < length) {
    if (compare(first[child2nd], first[child2nd - 1])) {
      --child2nd;
    }

    first[value_index] = etl::move(first[child2nd]);
    value_index = child2nd;
    child2nd = 2 * (child2nd + 1);
  }

  if (child2nd == length) {
    first[value_index] = etl::move(first[child2nd - 1]);
    value_index = child2nd - 1;
  }

  push_heap(first, value_index, top_index, etl::move(value), compare);
}

template <typename TIterator, typename TDistance, typename TCompare>
bool is_heap(const TIterator first, const TDistance n, TCompare compare) {
  TDistance parent = 0;

  for (TDistance child = 1; child < n; ++child) {
    if (compare(first[parent], first[child])) {
      return false;
    }

    if ((child & 1) == 0) {
      ++parent;
    }
  }

  return true;
}
}  // namespace private_heap

template <typename TIterator, typename TCompare>
void pop_heap(TIterator first, TIterator last, TCompare compare) {
  typedef typename etl::iterator_traits<TIterator>::value_type value_t;
  typedef typename etl::iterator_traits<TIterator>::difference_type distance_t;

  value_t value = etl::move(last[-1]);
  last[-1] = etl::move(first[0]);

  private_heap::adjust_heap(first, distance_t(0), distance_t(last - first - 1),
                            etl::move(value), compare);
}

template <typename TIterator>
void pop_heap(TIterator first, TIterator last) {
  typedef etl::less<typename etl::iterator_traits<TIterator>::value_type>
      compare;

  etl::pop_heap(first, last, compare());
}

template <typename TIterator, typename TCompare>
void push_heap(TIterator first, TIterator last, TCompare compare) {
  typedef
      typename etl::iterator_traits<TIterator>::difference_type difference_t;
  typedef typename etl::iterator_traits<TIterator>::value_type value_t;

  private_heap::push_heap(first, difference_t(last - first - 1),
                          difference_t(0), value_t(etl::move(*(last - 1))),
                          compare);
}

template <typename TIterator>
void push_heap(TIterator first, TIterator last) {
  typedef etl::less<typename etl::iterator_traits<TIterator>::value_type>
      compare;

  etl::push_heap(first, last, compare());
}

template <typename TIterator, typename TCompare>
void make_heap(TIterator first, TIterator last, TCompare compare) {
  typedef
      typename etl::iterator_traits<TIterator>::difference_type difference_t;

  if ((last - first) < 2) {
    return;
  }

  difference_t length = last - first;
  difference_t parent = (length - 2) / 2;

  while (true) {
    private_heap::adjust_heap(first, parent, length,
                              etl::move(*(first + parent)), compare);

    if (parent == 0) {
      return;
    }

    --parent;
  }
}

template <typename TIterator>
void make_heap(TIterator first, TIterator last) {
  typedef etl::less<typename etl::iterator_traits<TIterator>::value_type>
      compare;

  etl::make_heap(first, last, compare());
}

template <typename TIterator>
[[nodiscard]] bool is_heap(TIterator first, TIterator last) {
  typedef etl::less<typename etl::iterator_traits<TIterator>::value_type>
      compare;

  return private_heap::is_heap(first, last - first, compare());
}

template <typename TIterator, typename TCompare>
[[nodiscard]] bool is_heap(TIterator first, TIterator last, TCompare compare) {
  return private_heap::is_heap(first, last - first, compare);
}

template <typename TIterator>
void sort_heap(TIterator first, TIterator last) {
  while (first != last) {
    etl::pop_heap(first, last);
    --last;
  }
}

template <typename TIterator, typename TCompare>
void sort_heap(TIterator first, TIterator last, TCompare compare) {
  while (first != last) {
    etl::pop_heap(first, last, compare);
    --last;
  }
}

template <typename TIterator1, typename TIterator2, typename TCompare>
[[nodiscard]] constexpr TIterator1 search(TIterator1 first,
                                          TIterator1 last,
                                          TIterator2 search_first,
                                          TIterator2 search_last,
                                          TCompare compare) {
  while (true) {
    TIterator1 itr = first;
    TIterator2 search_itr = search_first;

    while (true) {
      if (search_itr == search_last) {
        return first;
      }

      if (itr == last) {
        return last;
      }

      if (!compare(*itr, *search_itr)) {
        break;
      }

      ++itr;
      ++search_itr;
    }

    ++first;
  }
}

template <typename TIterator1, typename TIterator2>
[[nodiscard]] constexpr TIterator1 search(TIterator1 first,
                                          TIterator1 last,
                                          TIterator2 search_first,
                                          TIterator2 search_last) {
  typedef etl::equal_to<typename etl::iterator_traits<TIterator1>::value_type>
      compare;

  return etl::search(first, last, search_first, search_last, compare());
}

namespace private_algorithm {

template <typename TIterator>
constexpr TIterator rotate_general(TIterator first,
                                   TIterator middle,
                                   TIterator last) {
  TIterator next = middle;

  while (first != next) {
    using std::swap;

    swap(*first, *next);

    ++first;
    ++next;

    if (next == last) {
      next = middle;
    } else if (first == middle) {
      middle = next;
    }
  }

  return first;
}

template <typename TIterator>
constexpr TIterator rotate_left_by_one(TIterator first, TIterator last) {
  typedef typename etl::iterator_traits<TIterator>::value_type value_type;

  value_type temp(etl::move(*first));

  TIterator result = etl::move(etl::next(first), last, first);

  *result = etl::move(temp);

  return result;
}

template <typename TIterator>
constexpr TIterator rotate_right_by_one(TIterator first, TIterator last) {
  typedef typename etl::iterator_traits<TIterator>::value_type value_type;

  TIterator previous = etl::prev(last);
  value_type temp(etl::move(*previous));

  TIterator result = etl::move_backward(first, previous, last);

  *first = etl::move(temp);

  return result;
}
}  // namespace private_algorithm

template <typename TIterator>
constexpr TIterator rotate(TIterator first, TIterator middle, TIterator last) {
  if (etl::next(first) == middle) {
    return private_algorithm::rotate_left_by_one(first, last);
  }

  if (etl::next(middle) == last) {
    return private_algorithm::rotate_right_by_one(first, last);
  }

  return private_algorithm::rotate_general(first, middle, last);
}

template <typename TIterator1, typename TIterator2, typename TPredicate>
[[nodiscard]] constexpr TIterator1 find_end(TIterator1 b,
                                            TIterator1 e,
                                            TIterator2 sb,
                                            TIterator2 se,
                                            TPredicate predicate) {
  if (sb == se) {
    return e;
  }

  TIterator1 result = e;

  while (true) {
    TIterator1 new_result = etl::search(b, e, sb, se, predicate);

    if (new_result == e) {
      break;
    } else {
      result = new_result;
      b = result;
      ++b;
    }
  }
  return result;
}

template <typename TIterator1, typename TIterator2>
[[nodiscard]] constexpr TIterator1 find_end(TIterator1 b,
                                            TIterator1 e,
                                            TIterator2 sb,
                                            TIterator2 se) {
  typedef etl::equal_to<typename etl::iterator_traits<TIterator1>::value_type>
      predicate;

  return find_end(b, e, sb, se, predicate());
}

template <typename TIterator, typename TCompare>
[[nodiscard]] constexpr TIterator min_element(TIterator begin,
                                              TIterator end,
                                              TCompare compare) {
  TIterator minimum = begin;
  ++begin;

  while (begin != end) {
    if (compare(*begin, *minimum)) {
      minimum = begin;
    }

    ++begin;
  }

  return minimum;
}

template <typename TIterator>
[[nodiscard]] constexpr TIterator min_element(TIterator begin, TIterator end) {
  typedef typename etl::iterator_traits<TIterator>::value_type value_t;

  return etl::min_element(begin, end, etl::less<value_t>());
}

template <typename TIterator, typename TCompare>
[[nodiscard]] constexpr TIterator max_element(TIterator begin,
                                              TIterator end,
                                              TCompare compare) {
  TIterator maximum = begin;
  ++begin;

  while (begin != end) {
    if (!compare(*begin, *maximum)) {
      maximum = begin;
    }

    ++begin;
  }

  return maximum;
}

template <typename TIterator>
[[nodiscard]] constexpr TIterator max_element(TIterator begin, TIterator end) {
  typedef typename etl::iterator_traits<TIterator>::value_type value_t;

  return etl::max_element(begin, end, etl::less<value_t>());
}

template <typename TIterator, typename TCompare>
[[nodiscard]] constexpr std::pair<TIterator, TIterator> minmax_element(
    TIterator begin, TIterator end, TCompare compare) {
  TIterator minimum = begin;
  TIterator maximum = begin;
  ++begin;

  while (begin != end) {
    if (compare(*begin, *minimum)) {
      minimum = begin;
    }

    if (compare(*maximum, *begin)) {
      maximum = begin;
    }

    ++begin;
  }

  return std::pair<TIterator, TIterator>(minimum, maximum);
}

template <typename TIterator>
[[nodiscard]] constexpr std::pair<TIterator, TIterator> minmax_element(
    TIterator begin, TIterator end) {
  typedef typename etl::iterator_traits<TIterator>::value_type value_t;

  return etl::minmax_element(begin, end, etl::less<value_t>());
}

template <typename T>
[[nodiscard]] constexpr std::pair<const T&, const T&> minmax(const T& a,
                                                             const T& b) {
  return (b < a) ? std::pair<const T&, const T&>(b, a)
                 : std::pair<const T&, const T&>(a, b);
}

template <typename T, typename TCompare>
[[nodiscard]] constexpr std::pair<const T&, const T&> minmax(const T& a,
                                                             const T& b,
                                                             TCompare compare) {
  return compare(b, a) ? std::pair<const T&, const T&>(b, a)
                       : std::pair<const T&, const T&>(a, b);
}

template <typename TIterator>
[[nodiscard]] constexpr TIterator is_sorted_until(TIterator begin,
                                                  TIterator end) {
  if (begin != end) {
    TIterator next = begin;

    while (++next != end) {
      if (*next < *begin) {
        return next;
      }

      ++begin;
    }
  }

  return end;
}

template <typename TIterator, typename TCompare>
[[nodiscard]] constexpr TIterator is_sorted_until(TIterator begin,
                                                  TIterator end,
                                                  TCompare compare) {
  if (begin != end) {
    TIterator next = begin;

    while (++next != end) {
      if (compare(*next, *begin)) {
        return next;
      }

      ++begin;
    }
  }

  return end;
}

template <typename TIterator>
[[nodiscard]] constexpr bool is_sorted(TIterator begin, TIterator end) {
  return etl::is_sorted_until(begin, end) == end;
}

template <typename TIterator, typename TCompare>
[[nodiscard]] constexpr bool is_sorted(TIterator begin,
                                       TIterator end,
                                       TCompare compare) {
  return etl::is_sorted_until(begin, end, compare) == end;
}

template <typename TIterator, typename TUnaryPredicate>
[[nodiscard]] constexpr TIterator find_if_not(TIterator begin,
                                              TIterator end,
                                              TUnaryPredicate predicate) {
  while (begin != end) {
    if (!predicate(*begin)) {
      return begin;
    }

    ++begin;
  }

  return end;
}

template <typename TIterator1, typename TIterator2>
[[nodiscard]] constexpr bool is_permutation(TIterator1 begin1,
                                            TIterator1 end1,
                                            TIterator2 begin2) {
  if (begin1 != end1) {
    TIterator2 end2 = begin2;

    etl::advance(end2, etl::distance(begin1, end1));

    for (TIterator1 i = begin1; i != end1; ++i) {
      if (i == etl::find(begin1, i, *i)) {
        size_t n = etl::count(begin2, end2, *i);

        if (n == 0 || size_t(etl::count(i, end1, *i)) != n) {
          return false;
        }
      }
    }
  }

  return true;
}

template <typename TIterator1, typename TIterator2, typename TBinaryPredicate>
[[nodiscard]] constexpr bool is_permutation(TIterator1 begin1,
                                            TIterator1 end1,
                                            TIterator2 begin2,
                                            TBinaryPredicate predicate) {
  if (begin1 != end1) {
    TIterator2 end2 = begin2;

    etl::advance(end2, etl::distance(begin1, end1));

    for (TIterator1 i = begin1; i != end1; ++i) {
      if (i == etl::find_if(begin1, i, etl::bind1st(predicate, *i))) {
        size_t n = etl::count(begin2, end2, *i);

        if (n == 0 || size_t(etl::count(i, end1, *i)) != n) {
          return false;
        }
      }
    }
  }

  return true;
}

template <typename TIterator1, typename TIterator2>
[[nodiscard]] constexpr bool is_permutation(TIterator1 begin1,
                                            TIterator1 end1,
                                            TIterator2 begin2,
                                            TIterator2 end2) {
  if (begin1 != end1) {
    for (TIterator1 i = begin1; i != end1; ++i) {
      if (i == etl::find(begin1, i, *i)) {
        size_t n = etl::count(begin2, end2, *i);

        if (n == 0 || size_t(etl::count(i, end1, *i)) != n) {
          return false;
        }
      }
    }
  }

  return true;
}

template <typename TIterator1, typename TIterator2, typename TBinaryPredicate>
[[nodiscard]] bool is_permutation(TIterator1 begin1,
                                  TIterator1 end1,
                                  TIterator2 begin2,
                                  TIterator2 end2,
                                  TBinaryPredicate predicate) {
  if (begin1 != end1) {
    for (TIterator1 i = begin1; i != end1; ++i) {
      if (i == etl::find_if(begin1, i, etl::bind1st(predicate, *i))) {
        size_t n = etl::count(begin2, end2, *i);

        if (n == 0 || size_t(etl::count(i, end1, *i)) != n) {
          return false;
        }
      }
    }
  }

  return true;
}

template <typename TIterator, typename TUnaryPredicate>
[[nodiscard]] constexpr bool is_partitioned(TIterator begin,
                                            TIterator end,
                                            TUnaryPredicate predicate) {
  while (begin != end) {
    if (!predicate(*begin)) {
      break;
    }

    ++begin;
  }

  while (begin != end) {
    if (predicate(*begin)) {
      return false;
    }

    ++begin;
  }

  return true;
}

template <typename TIterator, typename TUnaryPredicate>
[[nodiscard]] constexpr TIterator partition_point(TIterator begin,
                                                  TIterator end,
                                                  TUnaryPredicate predicate) {
  while (begin != end) {
    if (!predicate(*begin)) {
      return begin;
    }

    ++begin;
  }

  return begin;
}

template <typename TSource,
          typename TDestinationTrue,
          typename TDestinationFalse,
          typename TUnaryPredicate>
constexpr std::pair<TDestinationTrue, TDestinationFalse> partition_copy(
    TSource begin,
    TSource end,
    TDestinationTrue destination_true,
    TDestinationFalse destination_false,
    TUnaryPredicate predicate) {
  while (begin != end) {
    if (predicate(*begin)) {
      *destination_true = *begin;
      ++destination_true;
    } else {
      *destination_false = *begin;
      ++destination_false;
    }

    ++begin;
  }

  return std::pair<TDestinationTrue, TDestinationFalse>(destination_true,
                                                        destination_false);
}

template <typename TIterator,
          typename TOutputIterator,
          typename TUnaryPredicate>
constexpr TOutputIterator copy_if(TIterator begin,
                                  TIterator end,
                                  TOutputIterator out,
                                  TUnaryPredicate predicate) {
  while (begin != end) {
    if (predicate(*begin)) {
      *out = *begin;
      ++out;
    }

    ++begin;
  }

  return out;
}

template <typename TIterator, typename TUnaryPredicate>
[[nodiscard]] constexpr bool all_of(TIterator begin,
                                    TIterator end,
                                    TUnaryPredicate predicate) {
  return etl::find_if_not(begin, end, predicate) == end;
}

template <typename TIterator, typename TUnaryPredicate>
[[nodiscard]] constexpr bool any_of(TIterator begin,
                                    TIterator end,
                                    TUnaryPredicate predicate) {
  return etl::find_if(begin, end, predicate) != end;
}

template <typename TIterator, typename TUnaryPredicate>
[[nodiscard]] constexpr bool none_of(TIterator begin,
                                     TIterator end,
                                     TUnaryPredicate predicate) {
  return etl::find_if(begin, end, predicate) == end;
}
template <typename TIterator, typename TCompare>
void sort(TIterator first, TIterator last, TCompare compare) {
  std::sort(first, last, compare);
}

template <typename TIterator>
void sort(TIterator first, TIterator last) {
  std::sort(first, last);
}

template <typename TIterator, typename TCompare>
void stable_sort(TIterator first, TIterator last, TCompare compare) {
  std::stable_sort(first, last, compare);
}

template <typename TIterator>
void stable_sort(TIterator first, TIterator last) {
  std::stable_sort(first, last);
}

template <typename TIterator, typename T>
constexpr T accumulate(TIterator first, TIterator last, T sum) {
  while (first != last) {
    sum = etl::move(sum) + *first;
    ++first;
  }

  return sum;
}

template <typename TIterator, typename T, typename TBinaryOperation>
constexpr T accumulate(TIterator first,
                       TIterator last,
                       T sum,
                       TBinaryOperation operation) {
  while (first != last) {
    sum = operation(etl::move(sum), *first);
    ++first;
  }

  return sum;
}

template <typename T, typename TCompare>
constexpr const T& clamp(const T& value,
                         const T& low,
                         const T& high,
                         TCompare compare) {
  return compare(value, low) ? low : compare(high, value) ? high : value;
}

template <typename T>
constexpr const T& clamp(const T& value, const T& low, const T& high) {
  return clamp(value, low, high, etl::less<T>());
}

template <typename TIterator, typename T>
constexpr TIterator remove(TIterator first, TIterator last, const T& value) {
  first = etl::find(first, last, value);

  if (first != last) {
    TIterator itr = first;

    while (itr != last) {
      if (!(*itr == value)) {
        *first = etl::move(*itr);
        ++first;
      }

      ++itr;
    }
  }

  return first;
}

template <typename TIterator, typename TUnaryPredicate>
constexpr TIterator remove_if(TIterator first,
                              TIterator last,
                              TUnaryPredicate predicate) {
  first = etl::find_if(first, last, predicate);

  if (first != last) {
    TIterator itr = first;

    while (itr != last) {
      if (!predicate(*itr)) {
        *first = etl::move(*itr);
        ++first;
      }

      ++itr;
    }
  }

  return first;
}
}  // namespace etl

namespace etl {
template <typename TInputIterator, typename TOutputIterator>
constexpr
    typename etl::enable_if<etl::is_random_iterator<TInputIterator>::value &&
                                etl::is_random_iterator<TOutputIterator>::value,
                            TOutputIterator>::type
    copy_s(TInputIterator i_begin,
           TInputIterator i_end,
           TOutputIterator o_begin,
           TOutputIterator o_end) {
  size_t s_size = etl::distance(i_begin, i_end);
  size_t d_size = etl::distance(o_begin, o_end);
  size_t size = (s_size < d_size) ? s_size : d_size;

  return etl::copy(i_begin, i_begin + size, o_begin);
}
template <typename TInputIterator, typename TOutputIterator>
constexpr typename etl::enable_if<
    !etl::is_random_iterator<TInputIterator>::value ||
        !etl::is_random_iterator<TOutputIterator>::value,
    TOutputIterator>::type
copy_s(TInputIterator i_begin,
       TInputIterator i_end,
       TOutputIterator o_begin,
       TOutputIterator o_end) {
  while ((i_begin != i_end) && (o_begin != o_end)) {
    *o_begin = *i_begin;
    ++o_begin;
    ++i_begin;
  }

  return o_begin;
}

template <typename TInputIterator, typename TSize, typename TOutputIterator>
constexpr TOutputIterator copy_n_s(TInputIterator i_begin,
                                   TSize n,
                                   TOutputIterator o_begin,
                                   TOutputIterator o_end) {
  while ((n-- > 0) && (o_begin != o_end)) {
    *o_begin = *i_begin;
    ++o_begin;
    ++i_begin;
  }

  return o_begin;
}

template <typename TInputIterator,
          typename TSize1,
          typename TOutputIterator,
          typename TSize2>
constexpr TOutputIterator copy_n_s(TInputIterator i_begin,
                                   TSize1 n1,
                                   TOutputIterator o_begin,
                                   TSize2 n2) {
  while ((n1-- > 0) && (n2-- > 0)) {
    *o_begin = *i_begin;
    ++o_begin;
    ++i_begin;
  }

  return o_begin;
}

template <typename TInputIterator,
          typename TOutputIterator,
          typename TUnaryPredicate>
constexpr TOutputIterator copy_if_s(TInputIterator i_begin,
                                    TInputIterator i_end,
                                    TOutputIterator o_begin,
                                    TOutputIterator o_end,
                                    TUnaryPredicate predicate) {
  while ((i_begin != i_end) && (o_begin != o_end)) {
    if (predicate(*i_begin)) {
      *o_begin = *i_begin;
      ++o_begin;
    }

    ++i_begin;
  }

  return o_begin;
}

template <typename TInputIterator,
          typename TSize,
          typename TOutputIterator,
          typename TUnaryPredicate>
constexpr TOutputIterator copy_n_if(TInputIterator i_begin,
                                    TSize n,
                                    TOutputIterator o_begin,
                                    TUnaryPredicate predicate) {
  while (n-- > 0) {
    if (predicate(*i_begin)) {
      *o_begin = *i_begin;
      ++o_begin;
    }

    ++i_begin;
  }

  return o_begin;
}
template <typename TInputIterator, typename TOutputIterator>
constexpr
    typename etl::enable_if<etl::is_random_iterator<TInputIterator>::value &&
                                etl::is_random_iterator<TOutputIterator>::value,
                            TOutputIterator>::type
    move_s(TInputIterator i_begin,
           TInputIterator i_end,
           TOutputIterator o_begin,
           TOutputIterator o_end) {
  size_t s_size = etl::distance(i_begin, i_end);
  size_t d_size = etl::distance(o_begin, o_end);
  size_t size = (s_size < d_size) ? s_size : d_size;

  return etl::move(i_begin, i_begin + size, o_begin);
}
template <typename TInputIterator, typename TOutputIterator>
constexpr typename etl::enable_if<
    !etl::is_random_iterator<TInputIterator>::value ||
        !etl::is_random_iterator<TOutputIterator>::value,
    TOutputIterator>::type
move_s(TInputIterator i_begin,
       TInputIterator i_end,
       TOutputIterator o_begin,
       TOutputIterator o_end) {
  while ((i_begin != i_end) && (o_begin != o_end)) {
    *o_begin = etl::move(*i_begin);
    ++i_begin;
    ++o_begin;
  }

  return o_begin;
}
template <typename TIterator, typename TValue>
[[nodiscard]] constexpr TIterator binary_find(TIterator begin,
                                              TIterator end,
                                              const TValue& value) {
  TIterator it = etl::lower_bound(begin, end, value);

  if ((it == end) || (*it != value)) {
    it = end;
  }

  return it;
}

template <typename TIterator,
          typename TValue,
          typename TBinaryPredicate,
          typename TBinaryEquality>
[[nodiscard]] constexpr TIterator binary_find(TIterator begin,
                                              TIterator end,
                                              const TValue& value,
                                              TBinaryPredicate predicate,
                                              TBinaryEquality equality) {
  TIterator it = etl::lower_bound(begin, end, value, predicate);

  if ((it == end) || !equality(*it, value)) {
    it = end;
  }

  return it;
}

template <typename TIterator, typename TUnaryFunction, typename TUnaryPredicate>
constexpr TUnaryFunction for_each_if(TIterator begin,
                                     const TIterator end,
                                     TUnaryFunction function,
                                     TUnaryPredicate predicate) {
  while (begin != end) {
    if (predicate(*begin)) {
      function(*begin);
    }

    ++begin;
  }

  return function;
}

template <typename TIterator, typename TSize, typename TUnaryFunction>
constexpr TIterator for_each_n(TIterator begin,
                               TSize n,
                               TUnaryFunction function) {
  while (n-- > 0) {
    function(*begin);
    ++begin;
  }

  return begin;
}

template <typename TIterator,
          typename TSize,
          typename TUnaryFunction,
          typename TUnaryPredicate>
constexpr TIterator for_each_n_if(TIterator begin,
                                  TSize n,
                                  TUnaryFunction function,
                                  TUnaryPredicate predicate) {
  while (n-- > 0) {
    if (predicate(*begin)) {
      function(*begin);
    }

    ++begin;
  }

  return begin;
}

template <typename TInputIterator,
          typename TOutputIterator,
          typename TUnaryFunction>
constexpr TOutputIterator transform_s(TInputIterator i_begin,
                                      TInputIterator i_end,
                                      TOutputIterator o_begin,
                                      TOutputIterator o_end,
                                      TUnaryFunction function) {
  while ((i_begin != i_end) && (o_begin != o_end)) {
    *o_begin = function(*i_begin);
    ++i_begin;
    ++o_begin;
  }

  return o_begin;
}

template <typename TInputIterator,
          typename TSize,
          typename TOutputIterator,
          typename TUnaryFunction>
constexpr void transform_n(TInputIterator i_begin,
                           TSize n,
                           TOutputIterator o_begin,
                           TUnaryFunction function) {
  TInputIterator i_end(i_begin);
  etl::advance(i_end, n);

  etl::transform(i_begin, i_end, o_begin, function);
}

template <typename TInputIterator1,
          typename TInputIterator2,
          typename TSize,
          typename TOutputIterator,
          typename TBinaryFunction>
constexpr void transform_n(TInputIterator1 i_begin1,
                           TInputIterator2 i_begin2,
                           TSize n,
                           TOutputIterator o_begin,
                           TBinaryFunction function) {
  TInputIterator1 i_end1(i_begin1);
  etl::advance(i_end1, n);

  etl::transform(i_begin1, i_end1, i_begin2, o_begin, function);
}

template <typename TInputIterator,
          typename TOutputIterator,
          typename TUnaryFunction,
          typename TUnaryPredicate>
constexpr TOutputIterator transform_if(TInputIterator i_begin,
                                       const TInputIterator i_end,
                                       TOutputIterator o_begin,
                                       TUnaryFunction function,
                                       TUnaryPredicate predicate) {
  while (i_begin != i_end) {
    if (predicate(*i_begin)) {
      *o_begin = function(*i_begin);
      ++o_begin;
    }

    ++i_begin;
  }

  return o_begin;
}

template <typename TInputIterator1,
          typename TInputIterator2,
          typename TOutputIterator,
          typename TBinaryFunction,
          typename TBinaryPredicate>
constexpr TOutputIterator transform_if(TInputIterator1 i_begin1,
                                       const TInputIterator1 i_end1,
                                       TInputIterator2 i_begin2,
                                       TOutputIterator o_begin,
                                       TBinaryFunction function,
                                       TBinaryPredicate predicate) {
  while (i_begin1 != i_end1) {
    if (predicate(*i_begin1, *i_begin2)) {
      *o_begin = function(*i_begin1, *i_begin2);
      ++o_begin;
    }

    ++i_begin1;
    ++i_begin2;
  }

  return o_begin;
}

template <typename TInputIterator,
          typename TSize,
          typename TOutputIterator,
          typename TUnaryFunction,
          typename TUnaryPredicate>
constexpr TOutputIterator transform_n_if(TInputIterator i_begin,
                                         TSize n,
                                         TOutputIterator o_begin,
                                         TUnaryFunction function,
                                         TUnaryPredicate predicate) {
  while (n-- > 0) {
    if (predicate(*i_begin)) {
      *o_begin = function(*i_begin);
      ++o_begin;
    }

    ++i_begin;
  }

  return o_begin;
}

template <typename TInputIterator1,
          typename TInputIterator2,
          typename TSize,
          typename TOutputIterator,
          typename TBinaryFunction,
          typename TBinaryPredicate>
constexpr TOutputIterator transform_n_if(TInputIterator1 i_begin1,
                                         TInputIterator2 i_begin2,
                                         TSize n,
                                         TOutputIterator o_begin,
                                         TBinaryFunction function,
                                         TBinaryPredicate predicate) {
  while (n-- > 0) {
    if (predicate(*i_begin1, *i_begin2)) {
      *o_begin++ = function(*i_begin1, *i_begin2);
    }

    ++i_begin1;
    ++i_begin2;
  }

  return o_begin;
}

template <typename TSource,
          typename TDestinationTrue,
          typename TDestinationFalse,
          typename TUnaryFunctionTrue,
          typename TUnaryFunctionFalse,
          typename TUnaryPredicate>
constexpr std::pair<TDestinationTrue, TDestinationFalse> partition_transform(
    TSource begin,
    TSource end,
    TDestinationTrue destination_true,
    TDestinationFalse destination_false,
    TUnaryFunctionTrue function_true,
    TUnaryFunctionFalse function_false,
    TUnaryPredicate predicate) {
  while (begin != end) {
    if (predicate(*begin)) {
      *destination_true = function_true(*begin);
      ++destination_true;
    } else {
      *destination_false = function_false(*begin);
      ++destination_false;
    }

    ++begin;
  }

  return std::pair<TDestinationTrue, TDestinationFalse>(destination_true,
                                                        destination_false);
}

template <typename TSource1,
          typename TSource2,
          typename TDestinationTrue,
          typename TDestinationFalse,
          typename TBinaryFunctionTrue,
          typename TBinaryFunctionFalse,
          typename TBinaryPredicate>
constexpr std::pair<TDestinationTrue, TDestinationFalse> partition_transform(
    TSource1 begin1,
    TSource1 end1,
    TSource2 begin2,
    TDestinationTrue destination_true,
    TDestinationFalse destination_false,
    TBinaryFunctionTrue function_true,
    TBinaryFunctionFalse function_false,
    TBinaryPredicate predicate) {
  while (begin1 != end1) {
    if (predicate(*begin1, *begin2)) {
      *destination_true = function_true(*begin1, *begin2);
      ++destination_true;
    } else {
      *destination_false = function_false(*begin1, *begin2);
      ++destination_false;
    }

    ++begin1;
    ++begin2;
  }

  return std::pair<TDestinationTrue, TDestinationFalse>(destination_true,
                                                        destination_false);
}

template <typename TIterator, typename TCompare>

void shell_sort(TIterator first, TIterator last, TCompare compare) {
  if (first == last) {
    return;
  }

  typedef
      typename etl::iterator_traits<TIterator>::difference_type difference_t;

  difference_t n = etl::distance(first, last);

  for (difference_t i = n / 2; i > 0; i /= 2) {
    for (difference_t j = i; j < n; ++j) {
      for (difference_t k = j - i; k >= 0; k -= i) {
        TIterator itr1 = first;
        TIterator itr2 = first;

        etl::advance(itr1, k);
        etl::advance(itr2, k + i);

        if (compare(*itr2, *itr1)) {
          etl::iter_swap(itr1, itr2);
        }
      }
    }
  }
}

template <typename TIterator>

void shell_sort(TIterator first, TIterator last) {
  etl::shell_sort(
      first, last,
      etl::less<typename etl::iterator_traits<TIterator>::value_type>());
}

template <typename TIterator, typename TCompare>
constexpr void insertion_sort(TIterator first,
                              TIterator last,
                              TCompare compare) {
  for (TIterator itr = first; itr != last; ++itr) {
    etl::rotate(etl::upper_bound(first, itr, *itr, compare), itr,
                etl::next(itr));
  }
}

template <typename TIterator>
constexpr void insertion_sort(TIterator first, TIterator last) {
  etl::insertion_sort(
      first, last,
      etl::less<typename etl::iterator_traits<TIterator>::value_type>());
}

namespace private_algorithm {
template <typename TIterator>
constexpr typename etl::enable_if<etl::is_forward_iterator<TIterator>::value,
                                  TIterator>::type
get_before_last(TIterator first_, TIterator last_) {
  TIterator last = first_;
  TIterator lastplus1 = first_;
  ++lastplus1;

  while (lastplus1 != last_) {
    ++last;
    ++lastplus1;
  }

  return last;
}

template <typename TIterator>
constexpr
    typename etl::enable_if<etl::is_bidirectional_iterator<TIterator>::value,
                            TIterator>::type
    get_before_last(TIterator, TIterator last_) {
  TIterator last = last_;
  --last;

  return last;
}

template <typename TIterator>
constexpr
    typename etl::enable_if<etl::is_random_access_iterator<TIterator>::value,
                            TIterator>::type
    get_before_last(TIterator, TIterator last_) {
  return last_ - 1;
}
}  // namespace private_algorithm

template <typename TIterator, typename TCompare>

void selection_sort(TIterator first, TIterator last, TCompare compare) {
  TIterator min;
  const TIterator ilast = private_algorithm::get_before_last(first, last);
  const TIterator jlast = last;

  for (TIterator i = first; i != ilast; ++i) {
    min = i;

    TIterator j = i;
    ++j;

    for (; j != jlast; ++j) {
      if (compare(*j, *min)) {
        min = j;
      }
    }

    using std::swap;
    swap(*i, *min);
  }
}

template <typename TIterator>

void selection_sort(TIterator first, TIterator last) {
  selection_sort(
      first, last,
      etl::less<typename etl::iterator_traits<TIterator>::value_type>());
}

template <typename TIterator, typename TCompare>
constexpr void heap_sort(TIterator first, TIterator last, TCompare compare) {
  if (!etl::is_heap(first, last, compare)) {
    etl::make_heap(first, last, compare);
  }

  etl::sort_heap(first, last, compare);
}

template <typename TIterator>
constexpr void heap_sort(TIterator first, TIterator last) {
  if (!etl::is_heap(first, last)) {
    etl::make_heap(first, last);
  }

  etl::sort_heap(first, last);
}

template <typename T>
[[nodiscard]] constexpr const T& multimax(const T& a, const T& b) {
  return a < b ? b : a;
}

template <typename T, typename... Tx>
[[nodiscard]] constexpr const T& multimax(const T& t, const Tx&... tx) {
  return multimax(t, multimax(tx...));
}

template <typename TCompare, typename T>
[[nodiscard]] constexpr const T& multimax_compare(TCompare compare,
                                                  const T& a,
                                                  const T& b) {
  return compare(a, b) ? b : a;
}

template <typename TCompare, typename T, typename... Tx>
[[nodiscard]] constexpr const T& multimax_compare(TCompare compare,
                                                  const T& t,
                                                  const Tx&... tx) {
  return multimax_compare(compare, t, multimax_compare(compare, tx...));
}

template <typename T>
[[nodiscard]] constexpr const T& multimin(const T& a, const T& b) {
  return a < b ? a : b;
}

template <typename T, typename... Tx>
[[nodiscard]] constexpr const T& multimin(const T& t, const Tx&... tx) {
  return multimin(t, multimin(tx...));
}

template <typename TCompare, typename T>
[[nodiscard]] constexpr const T& multimin_compare(TCompare compare,
                                                  const T& a,
                                                  const T& b) {
  return compare(a, b) ? a : b;
}

template <typename TCompare, typename T, typename... Tx>
[[nodiscard]] constexpr const T& multimin_compare(TCompare compare,
                                                  const T& t,
                                                  const Tx&... tx) {
  return multimin_compare(compare, t, multimin_compare(compare, tx...));
}

template <typename TIterator>
[[nodiscard]] constexpr const TIterator& multimax_iter(const TIterator& a,
                                                       const TIterator& b) {
  return *a < *b ? b : a;
}

template <typename TIterator, typename... TIteratorx>
[[nodiscard]] constexpr const TIterator& multimax_iter(
    const TIterator& t, const TIteratorx&... tx) {
  return multimax_iter(t, multimax_iter(tx...));
}

template <typename TCompare, typename TIterator>
[[nodiscard]] constexpr const TIterator& multimax_iter_compare(
    TCompare compare, const TIterator& a, const TIterator& b) {
  return compare(*a, *b) ? b : a;
}

template <typename TCompare, typename TIterator, typename... TIteratorx>
[[nodiscard]] constexpr const TIterator& multimax_iter_compare(
    TCompare compare, const TIterator& t, const TIteratorx&... tx) {
  return multimax_iter_compare(compare, t,
                               multimax_iter_compare(compare, tx...));
}

template <typename TIterator>
[[nodiscard]] constexpr const TIterator& multimin_iter(const TIterator& a,
                                                       const TIterator& b) {
  return *a < *b ? a : b;
}

template <typename TIterator, typename... Tx>
[[nodiscard]] constexpr const TIterator& multimin_iter(const TIterator& t,
                                                       const Tx&... tx) {
  return multimin_iter(t, multimin_iter(tx...));
}

template <typename TCompare, typename TIterator>
[[nodiscard]] constexpr const TIterator& multimin_iter_compare(
    TCompare compare, const TIterator& a, const TIterator& b) {
  return compare(*a, *b) ? a : b;
}

template <typename TCompare, typename TIterator, typename... Tx>
[[nodiscard]] constexpr const TIterator& multimin_iter_compare(
    TCompare compare, const TIterator& t, const Tx&... tx) {
  return multimin_iter_compare(compare, t,
                               multimin_iter_compare(compare, tx...));
}

}  // namespace etl

namespace etl {
namespace private_alignment {

template <const bool IS_MATCH,
          const size_t ALIGNMENT,
          typename T1 = void,
          typename T2 = void,
          typename T3 = void,
          typename T4 = void,
          typename T5 = void,
          typename T6 = void,
          typename T7 = void,
          typename T8 = void>
class type_with_alignment_matcher;

template <const size_t ALIGNMENT,
          typename T1,
          typename T2,
          typename T3,
          typename T4,
          typename T5,
          typename T6,
          typename T7,
          typename T8>
class type_with_alignment_matcher<true,
                                  ALIGNMENT,
                                  T1,
                                  T2,
                                  T3,
                                  T4,
                                  T5,
                                  T6,
                                  T7,
                                  T8> {
 public:
  typedef T1 type;
};

template <const size_t ALIGNMENT,
          typename T1,
          typename T2,
          typename T3,
          typename T4,
          typename T5,
          typename T6,
          typename T7,
          typename T8>
class type_with_alignment_matcher<false,
                                  ALIGNMENT,
                                  T1,
                                  T2,
                                  T3,
                                  T4,
                                  T5,
                                  T6,
                                  T7,
                                  T8> {
 public:
  typedef typename type_with_alignment_matcher<ALIGNMENT <=
                                                   etl::alignment_of<T2>::value,
                                               ALIGNMENT,
                                               T2,
                                               T3,
                                               T4,
                                               T5,
                                               T6,
                                               T7,
                                               T8,
                                               void>::type type;
};

template <const size_t ALIGNMENT>
class type_with_alignment_matcher<false,
                                  ALIGNMENT,
                                  void,
                                  void,
                                  void,
                                  void,
                                  void,
                                  void,
                                  void,
                                  void> {
 public:
  typedef char type;
};

template <const size_t ALIGNMENT,
          typename T1,
          typename T2 = void,
          typename T3 = void,
          typename T4 = void,
          typename T5 = void,
          typename T6 = void,
          typename T7 = void,
          typename T8 = void>
class type_with_alignment_helper {
 public:
  typedef typename type_with_alignment_matcher<ALIGNMENT <=
                                                   etl::alignment_of<T1>::value,
                                               ALIGNMENT,
                                               T1,
                                               T2,
                                               T3,
                                               T4,
                                               T5,
                                               T6,
                                               T7,
                                               T8>::type type;
};
}  // namespace private_alignment

template <const size_t ALIGNMENT>
class type_with_alignment {
 public:
  typedef
      typename private_alignment::type_with_alignment_helper<ALIGNMENT,
                                                             int_least8_t,
                                                             int_least16_t,
                                                             int32_t,
                                                             int64_t,
                                                             float,
                                                             double,
                                                             void*>::type type;
};

template <const size_t LENGTH, const size_t ALIGNMENT>
struct aligned_storage {
  struct type {
    template <typename T>
    operator T&() {
      static_assert((etl::is_same<T*, void*>::value ||
                     ((ALIGNMENT % etl::alignment_of<T>::value) == 0)),
                    "Incompatible alignment");
      T* t = *this;
      return *t;
    }

    template <typename T>
    operator const T&() const {
      static_assert((etl::is_same<T*, void*>::value ||
                     ((ALIGNMENT % etl::alignment_of<T>::value) == 0)),
                    "Incompatible alignment");
      const T* t = *this;
      return *t;
    }

    template <typename T>
    operator T*() {
      static_assert((etl::is_same<T*, void*>::value ||
                     ((ALIGNMENT % etl::alignment_of<T>::value) == 0)),
                    "Incompatible alignment");
      return reinterpret_cast<T*>(data);
    }

    template <typename T>
    operator const T*() const {
      static_assert((etl::is_same<T*, void*>::value ||
                     ((ALIGNMENT % etl::alignment_of<T>::value) == 0)),
                    "Incompatible alignment");
      return reinterpret_cast<const T*>(data);
    }

    template <typename T>
    T& get_reference() {
      static_assert((etl::is_same<T*, void*>::value ||
                     ((ALIGNMENT % etl::alignment_of<T>::value) == 0)),
                    "Incompatible alignment");
      T* t = *this;
      return *t;
    }

    template <typename T>
    const T& get_reference() const {
      static_assert((etl::is_same<T*, void*>::value ||
                     ((ALIGNMENT % etl::alignment_of<T>::value) == 0)),
                    "Incompatible alignment");
      const T* t = *this;
      return *t;
    }

    template <typename T>
    T* get_address() {
      static_assert((etl::is_same<T*, void*>::value ||
                     ((ALIGNMENT % etl::alignment_of<T>::value) == 0)),
                    "Incompatible alignment");
      return reinterpret_cast<T*>(data);
    }

    template <typename T>
    const T* get_address() const {
      static_assert((etl::is_same<T*, void*>::value ||
                     ((ALIGNMENT % etl::alignment_of<T>::value) == 0)),
                    "Incompatible alignment");
      return reinterpret_cast<const T*>(data);
    }

    alignas(ALIGNMENT) char data[LENGTH];
  };
};

template <const size_t LENGTH, const size_t ALIGNMENT>
using aligned_storage_t = typename aligned_storage<LENGTH, ALIGNMENT>::type;

template <size_t LENGTH, typename T>
struct aligned_storage_as
    : public etl::aligned_storage<LENGTH, etl::alignment_of<T>::value> {};

template <size_t LENGTH, typename T>
using aligned_storage_as_t = typename aligned_storage_as<LENGTH, T>::type;

}  // namespace etl

namespace etl {

template <size_t VBlock_Size, size_t VAlignment, size_t VSize>
class fixed_sized_memory_block_allocator : public imemory_block_allocator {
 public:
  fixed_sized_memory_block_allocator() {}
};
}  // namespace etl

namespace etl {

typedef uint_least8_t message_id_t;

class imessage {
 public:
  virtual ~imessage() {}
};

template <etl::message_id_t ID_, typename TParent = etl::imessage>
class message : public TParent {
 public:
  enum { ID = ID_ };
};
}  // namespace etl

namespace etl {
class ireference_counted_message;

class ireference_counted_message_pool {
 public:
  virtual ~ireference_counted_message_pool() {}
};
}  // namespace etl

namespace etl {

typedef std::atomic<int> atomic_int;
typedef std::atomic<int32_t> atomic_int32_t;

}  // namespace etl

namespace etl {

class ireference_counter {
 public:
  virtual ~ireference_counter() {}
};

template <typename TCounter>
class reference_counter : public ireference_counter {
 public:
  reference_counter() : reference_count(0) {}

 private:
  TCounter reference_count;
};

template <>
class reference_counter<void> : public ireference_counter {
 public:
  reference_counter() {}
};

class ireference_counted_object {
 public:
  virtual ~ireference_counted_object() {}
};

template <typename TObject, typename TCounter>
class reference_counted_object : public etl::ireference_counted_object {
 public:
  typedef TObject value_type;
  typedef TCounter counter_type;

  reference_counted_object(const TObject& object_) : object(object_) {}

 private:
  reference_counted_object(const reference_counted_object&) = delete;
  reference_counted_object& operator=(const reference_counted_object&) = delete;

  TObject object;
  etl::reference_counter<TCounter> reference_counter;
};

}  // namespace etl

namespace etl {

class ireference_counted_message {
 public:
  virtual ~ireference_counted_message() {}
};

template <typename TMessage, typename TCounter>
class reference_counted_message : public etl::ireference_counted_message {
 public:
  typedef TMessage message_type;
  typedef TCounter counter_type;

  reference_counted_message(const TMessage& msg_,
                            etl::ireference_counted_message_pool& owner_)
      : rc_object(msg_), owner(owner_) {}

 private:
  etl::reference_counted_object<TMessage, TCounter> rc_object;
  etl::ireference_counted_message_pool& owner;
};

template <typename TMessage>
using atomic_counted_message =
    etl::reference_counted_message<TMessage, etl::atomic_int32_t>;

}  // namespace etl

namespace etl {

template <typename TCounter>
class reference_counted_message_pool
    : public etl::ireference_counted_message_pool {
 public:
  reference_counted_message_pool(
      etl::imemory_block_allocator& memory_block_allocator_)
      : memory_block_allocator(memory_block_allocator_) {}

  template <typename TMessage1, typename... TMessages>
  struct pool_message_parameters {
   private:
    static constexpr size_t size1 =
        sizeof(etl::reference_counted_message<TMessage1, TCounter>);

    static constexpr size_t size2 =
        pool_message_parameters<TMessages...>::max_size;

    static constexpr size_t alignment1 = etl::alignment_of<
        etl::reference_counted_message<TMessage1, TCounter>>::value;

    static constexpr size_t alignment2 =
        pool_message_parameters<TMessages...>::max_alignment;

   public:
    static constexpr size_t max_size = (size1 < size2) ? size2 : size1;

    static constexpr size_t max_alignment =
        (alignment1 < alignment2) ? alignment2 : alignment1;
  };

  template <typename TMessage1>
  struct pool_message_parameters<TMessage1> {
   public:
    static constexpr size_t max_size =
        sizeof(etl::reference_counted_message<TMessage1, TCounter>);

    static constexpr size_t max_alignment = etl::alignment_of<
        etl::reference_counted_message<TMessage1, TCounter>>::value;
  };

 private:
  imemory_block_allocator& memory_block_allocator;

  reference_counted_message_pool(const reference_counted_message_pool&) =
      delete;
  reference_counted_message_pool& operator=(
      const reference_counted_message_pool&) = delete;
};

using atomic_counted_message_pool =
    reference_counted_message_pool<etl::atomic_int>;

}  // namespace etl

constexpr etl::message_id_t MessageId1 = 1U;
constexpr etl::message_id_t MessageId2 = 2U;

struct Message1 : public etl::message<MessageId1> {
  Message1(int i_) : i(i_) {}

  Message1(const Message1& oth) { i = oth.i; }

  ~Message1() override {}

  int i;
};

struct Message2 : public etl::message<MessageId2> {
  ~Message2() override {}
};

int main() {
  using pool_message_parameters =
      etl::atomic_counted_message_pool::pool_message_parameters<Message1,
                                                                Message2>;

  using pool_message_parameters =
      etl::atomic_counted_message_pool::pool_message_parameters<Message1,
                                                                Message2>;

  etl::fixed_sized_memory_block_allocator<
      pool_message_parameters::max_size, pool_message_parameters::max_alignment,
      4U>
      memory_allocator;

  etl::atomic_counted_message_pool message_pool(memory_allocator);

  Message1 message1(6);
  etl::reference_counted_message<Message1, etl::atomic_int> temp(message1,
                                                                 message_pool);
  const etl::ireference_counted_message& rcmessage = temp;
  rcmessage.~ireference_counted_message();
}
