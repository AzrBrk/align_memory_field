#pragma once
#include<iostream>
#include<tuple>
#include<variant>
#include<type_traits>
#include<string>
#include<functional>
#include<memory>

#define compilet_var static const
#define compilet_bool static const bool

using std::size_t;

struct shared_constructor;
template<class T>
struct ref_wrapper;

template<class ...Arg_t>
struct exp_list {
	static constexpr std::size_t length = sizeof...(Arg_t);
	template<template<class...> class TL> using to = TL<Arg_t...>;
};

template<class TL> struct to_exp_list {};

template<template<class ...> class TL, class ...Typs> struct to_exp_list<TL<Typs...>>
{
	using type = exp_list<Typs...>;
};
template<class T, class TL>
struct add_to_front {};

template<class T, template<class...> class TL, class ...L>
struct add_to_front<T, TL<L...>>
{
	using type = TL<T, L...>;
};

template<class TL>
struct type_list_size
{
	static const size_t value = 0;
};

template<template<class...> class TL, class ...L>
struct type_list_size<TL<L...>>
{
	static const size_t value = sizeof...(L);
};
template<class L>
using size_of_type_list = type_list_size<L>;

template<class L>
constexpr size_t exp_size = size_of_type_list<L>::value;

template<class T>
struct is_empty_list
{
	static const bool value = false;
};
template<template<class...> class TL>
struct is_empty_list<TL<>>
{
	static const bool value = true;
};


template<class T>
struct max_type_list_index
{
	//static_assert(exp_size<T> != 0, "Can not provide index from an empty list!");
	static constexpr auto value = size_of_type_list<T>::value - 1;
};

template<class TL> constexpr size_t max_index = max_type_list_index<TL>::value;

struct end_of_list {};

template<class L>
struct split_first
{
	using type = end_of_list;
	using rest = end_of_list;
};

template<template<class...> class TL, class T, class ...L>
struct split_first<TL<T, L...>>
{
	using first = T;
	using rest = TL<L...>;
	using type = TL<T, L...>;
};
template<template<class...> class TL>
struct split_first<TL<>>
{
	using first = end_of_list;
	using rest = end_of_list;
};

template<std::size_t I>
struct exp_index
{
	const static size_t value = I;
};

namespace experiment {
	template<bool Ignore, std::size_t Index, std::size_t NInc, class TL>
	struct erase_until
	{};
	template<std::size_t Index, std::size_t NInc, class TL>
	struct erase_until <true, Index, NInc, TL>
	{
		using type = typename erase_until<NInc != Index, Index, NInc + 1, typename split_first<TL>::rest>::type;
	};
	template<std::size_t Index, std::size_t NInc, class TL>
	struct erase_until <false, Index, NInc, TL>
	{
		using type = TL;
	};

	template<std::size_t NInc, class TL>
	struct erase_until<false, 0, NInc, TL>
	{
		using type = typename split_first<TL>::rest;
	};

	template<std::size_t Index, class TL>
	using exp_ignore_until = typename erase_until<Index != 0, Index, 0, TL>::type;


}


template<class T>
struct recover_list
{
	using first_type = typename split_first<T>::first;
	using rest = typename split_first<T>::rest;
	using type = typename split_first<T>::type;
};

template<std::size_t N, class T>
struct select_type
{
	static const size_t Max_Value = max_type_list_index<T>::value;
	static_assert((N <= Max_Value), "index overflow");

	using type_list = recover_list<T>;
	using first_type = typename type_list::first_type;
	using lesser_type = typename type_list::rest;
	using type = typename select_type<N - 1, lesser_type>::type;
};
template<class T>
struct select_type<0, T>
{
	using first_type = typename recover_list<T>::first_type;
	using type = first_type;
};

template<size_t Idx, class TL>
using exp_select = typename select_type<Idx, TL>::type;

template<class TL1, template<class...> class TL2>
struct exp_rename {};

template<template<class ...> class TL1, template<class ...> class TL2, class ...L>
struct exp_rename<TL1<L...>, TL2>
{
	using type = TL2<L...>;
};

template<size_t N, class C, class T>
struct exp_cmp
{
	static const bool value = std::is_same<C, typename select_type<N, T>::type>::value;
};

template<bool bsame, size_t N, class C, class T>
struct is_one_of_type
{};

template<std::size_t N, class C, class T>
struct is_one_of_type<true, N, C, T>
{
	static const bool value = true;
	static const size_t index = N;
};

template<std::size_t N, class C, class T>
struct is_one_of_type<false, N, C, T>
{
	static const bool value = is_one_of_type<exp_cmp<N - 1, C, T>::value, N - 1, C, T>::value;
	static const size_t index = is_one_of_type<exp_cmp<N - 1, C, T>::value, N - 1, C, T>::index;
};

template<class C, class T>
struct is_one_of_type<false, 0, C, T>
{
	static const bool value = false;
	//over bondages to fail the compilation, if a false result but is trying to fetch index
	static const size_t index = size_of_type_list<T>::value;
};



template<class C, class T>
using exp_is_one_of
= is_one_of_type <
	exp_cmp<max_type_list_index<T>::value, C, T>::value,
	max_type_list_index<T>::value,
	C, T>;


template<class L, class T>
struct exp_join_impl
{};

template<template<class...> class L, class T, class ...List>
struct exp_join_impl<L<List...>, T>
{
	using type = L<List..., T>;
};
template<class TL>
struct exp_empty
{};

template<template<class...> class TL, class ...L>
struct exp_empty < TL<L...>>
{
	using type = TL<>;
};
template<class L1, class L2>
struct exp_join
{};

template<class L1, template<class...> class L2, class T, class ...List>
struct exp_join<L1, L2<T, List...>>
{
	using join_type = typename exp_join_impl<L1, T>::type;
	using type = typename exp_join <join_type, L2<List...>>::type;
};
template<class L1, template<class...> class L2>
struct exp_join<L1, L2<>>
{
	using type = L1;
};
template<class ...L>
struct exp_join_a_lot
{};
template<class L1, class L2, class ...L>
struct exp_join_a_lot<L1, L2, L...>
{
	using join_type = typename exp_join<L1, L2>::type;
	using type = typename exp_join_a_lot<join_type, L...>::type;
};
template<class L1, class L2>
struct exp_join_a_lot<L1, L2>
{
	using type = typename exp_join<L1, L2>::type;
};
template<std::size_t Index, size_t Current_Idx, bool Equal_Length, class TL, class SUB>
struct sub_type_list_impl
{};

template<std::size_t Index, std::size_t Current_Idx, template<class...> class TL, class SUB, class T, class ...L>
struct sub_type_list_impl <Index, Current_Idx, false, TL<T, L...>, SUB>
{
	using current_type = typename exp_join_impl<SUB, T>::type;
	using type = typename sub_type_list_impl<Index, Current_Idx + 1, (Index == Current_Idx + 1), TL<L...>, current_type>::type;
};
template<size_t Index, size_t Current_Idx, template<class...> class TL, class SUB, class ...L>
struct sub_type_list_impl <Index, Current_Idx, true, TL<L...>, SUB>
{
	using type = SUB;
};

template<std::size_t Index, class TL>
struct Test_Index
{
	static_assert((Index <= max_type_list_index<TL>::value + 1), "Test Index Error :index overflow");
	static const size_t value = Index;
};



template<std::size_t Index, class TL>
using sub_type_list = typename sub_type_list_impl<Test_Index<(Index), TL>::value, 0, (Index) == 0, TL, typename exp_empty<TL>::type>::type;

namespace experiment {
	template<std::size_t Index, class mtl>
	struct erase_at_t
	{
		static_assert(Index <= max_type_list_index<mtl>::value);
		using front = sub_type_list<Index, mtl>;
		using back = exp_ignore_until<Index, mtl>;
		using type = typename exp_join<front, back>::type;
	};
	template<class mtl>
	struct erase_at_t<0, mtl>
	{
		using type = typename split_first<mtl>::rest;
	};
}



template<class T, template<class...> class F>
struct exp_apply
{};
template<template<class...> class L, template<class...> class F, class ...D>
struct exp_apply <L<D...>, F>
{
	using type = L<F<D>...>;
};


template<bool Is_Unique, size_t N, class L>
struct is_unique_type_list_t
{};
template<std::size_t N, class L>
struct is_unique_type_list_t<false, N, L>//if a type is unique in a list;
{
	using first_type = typename recover_list<L>::first_type;
	using rest_type = typename recover_list<L>::rest;
	static const bool value = is_unique_type_list_t<
		exp_is_one_of<first_type, rest_type>::value,//if first type is one of a type in rest types
		N - 1,
		rest_type
	>::value;
};

template<std::size_t N, class L>
struct is_unique_type_list_t<true, N, L>
{
	static const bool value = false; // if there is a same type, then false
};


template<class L>
struct is_unique_type_list_t<false, 0, L>
{
	static const bool value = true;
};

template<class L>
using is_unique_type_list = is_unique_type_list_t<false, max_type_list_index<L>::value, L>;




template<class T>
struct is_wrapped
{
	static const bool value = false;
	using type = T;
};

template<template<class> class wrapper, class T>
struct is_wrapped<wrapper<T>>
{
	static const bool value = true;
	using type = T;
};
template<class T>
struct is_reference_type { static const bool value = false; };
template<class T>
struct is_reference_type<T&> { static const bool value = true; };

template<class T, template<class...>class TL>
struct is_wrap_with
{
	using TE = typename exp_empty<T>::type;
	static const bool value = std::is_same<TE, TL<>>::value;
};
template<std::size_t Idx, class T>
struct inserter {
	static const int value = Idx;
	using type = T;
};
template<class TL, class Inserter>
struct insert_at_impl
{
	using front = sub_type_list<Inserter::value, TL>;
	using back = experiment::exp_ignore_until<Inserter::value - 1, TL>;
	using type = typename exp_join_a_lot<front,
		exp_list<typename Inserter::type>,
		back>::type;
};
template<class TL, template<size_t, class> class Inserter, class T>
struct insert_at_impl <TL, Inserter<0, T>>
{
	using type = typename add_to_front<T, TL>::type;
};

template<class TL, class Inserter>
using insert_at = typename insert_at_impl<TL, Inserter>::type;


template<bool cnd, class A, class B>
struct condition_select_impl {};
template<class A, class B>
struct condition_select_impl<true, A, B> { using type = A; };
template<class A, class B>
struct condition_select_impl<false, A, B> { using type = B; };
template<bool cnd, class A, class B>
using exp_if = typename condition_select_impl<cnd, A, B>::type;


template<class _idx_t, class T>
struct TL_inserter
{
	using type = inserter<_idx_t::value, T>;
	template<class TL>
	using apply = insert_at<TL, type>;
};

template<class _idx_t>
struct TL_deleter
{
	template<class TL> using apply = exp_list<exp_select<_idx_t::value, TL>, typename experiment::erase_at_t<_idx_t::value, TL>::type>;
};

template<std::size_t ..._indices>
struct exp_select_list
{
	template<class TL> using apply = exp_list<exp_select<_indices, TL>...>;
};
struct no_exist_type {};

template<class T, class U = std::void_t<>>
struct get_type_impl : std::false_type
{
	using type = no_exist_type;
};

template<class T>
struct get_type_impl<T, std::void_t<typename T::type>> : std::true_type
{
	using type = typename T::type;
};

template<class T>
using get_type = typename get_type_impl<T>::type;

template<class T>
constexpr bool has_type = get_type_impl<T>::value;

template<template<template<class...> class, class ...> class F, class def = std::false_type> struct no_impl_for_typelist
{
	template<class T, class ...Other_Types> struct impl { using type = def; };
	template<template<class...> class TL, class ...Args, class ...Other_Types> struct impl<TL<Args...>, Other_Types...>
	{
		using type = F<TL, Other_Types..., Args...>;
	};

	template<class T, class ...Other_Types> using apply = typename impl<T, Other_Types...>::type;
};
template<class TL> struct is_exp_list_based : std::false_type {};
template<template<class ...> class TL, class ...TS>
struct is_exp_list_based<TL<TS...>>
{
	static constexpr bool value = std::is_base_of_v<exp_list<TS...>, TL<TS...>>;
};
template<class TL> constexpr bool is_exp_list_based_v = is_exp_list_based<TL>::value;
template<typename exp_tl, template<typename ...> class temp> requires is_exp_list_based_v<exp_tl>
using exp_transform_to = typename exp_tl::template to<temp>;

template<template<class ...> class Tl, class F, class ...Typs, class ...Args>
constexpr std::size_t template_func_execute_launcher(Tl<Typs...>, F&& f, Args&&...args)
{
	(f.template operator()(Typs{}, std::forward<Args>(args)...), ...);
	return sizeof...(Typs);
}

namespace exp_repeat
{

	template<class F, class ...L>
	struct Meta_Invoke {
		using type = typename F::template apply<L...>;
	};

	template<class F, class ...L>
	using meta_invoke = typename Meta_Invoke<F, L...>::type;

	struct below_zero {
		static constexpr size_t value = 0;
	};

	template<std::size_t _I>
	struct Idx {
		static const size_t value = _I;
		constexpr std::size_t operator()()
		{
			return _I;
		}
	};

	template<class _Idx, size_t _I>
	struct Add_Idx {};

	template<std::size_t I> struct Add_Idx<below_zero, I>
	{
		using type = Idx<0>;
	};

	template<template<size_t> class _Idx, size_t _I_in_Idx, size_t _I>
	struct Add_Idx<_Idx<_I_in_Idx>, _I>
	{
		using type = _Idx<_I_in_Idx + _I>;
	};

	template<class _Idx, size_t _I>
	using add_idx_t = typename Add_Idx<_Idx, _I>::type;

	template<class _Idx>
	using inc_idx_t = add_idx_t<_Idx, 1>;


	template<size_t _stride_length>
	struct _Stride {};

	template<bool _Condition, class _Meta_Object, class F, class Arg_t>
	struct looper
	{
		using _current_type = meta_invoke<_Meta_Object, meta_invoke<F, Arg_t>>;
		template<class _Bool_Fn>
		struct apply
		{
			using type = typename looper<
				meta_invoke<_Bool_Fn, _Meta_Object>::value, _current_type,
				F, meta_invoke<F, Arg_t>
			>::template apply<_Bool_Fn>::type;
		};
	};
	template<class _Meta_Object, class F, class Arg_t>
	struct looper<false, _Meta_Object, F, Arg_t>
	{
		template<class _Bool_Fn> struct apply { using type = _Meta_Object; };
	};

	template<size_t _Limit>
	struct _Length_Less_Than
	{
		template<class TL> struct apply { static const bool value = (TL::length < _Limit); };
	};

	template<class TL>
	struct _Appendable_Meta_List
	{
		static const size_t length = size_of_type_list<TL>::value;
		using type = TL;
		template<class _Extent>
		using apply = _Appendable_Meta_List<typename exp_join_impl<TL, _Extent>::type>;
	};

	struct _Increase_Idx
	{
		template<class _Idx> using apply = inc_idx_t <_Idx>;
	};




	template<size_t NUM>
	using meta_iota = typename looper<(NUM != 0), _Appendable_Meta_List<exp_list<exp_repeat::Idx<0>>>,
		_Increase_Idx, exp_repeat::Idx<0>>::template apply<_Length_Less_Than<NUM>>::type::type;

	template<size_t ..._NUM>
	struct meta_array
	{
		using cv_typelist = exp_list<Idx<_NUM>...>;
		template<template<size_t...I> class integer_array_type>
		using to = integer_array_type<_NUM...>;
		template<size_t N> using at = exp_select<N, cv_typelist>;

		static constexpr size_t length = cv_typelist::length;
		static constexpr size_t sum = (0 + ... + _NUM);
	};

	template<class TL>
	struct _Meta_To_Array {};

	template<template<class...> class TL, size_t... _I>
	struct  _Meta_To_Array<TL<Idx<_I>...>>
	{
		using type = meta_array<_I...>;
	};

	template<class TL>
	using meta_to_array = typename _Meta_To_Array<TL>::type;



	template<class ...>
	struct Meta_Expr {};
	template<class _Idx1, class _OP, class _Idx2>
	struct Meta_Expr<_Idx1, _OP, _Idx2> {
		using type = meta_invoke<_OP, _Idx1, _Idx2>;
	};
	namespace operators {

		template<const char _O> struct Meta_Op {};
		template<> struct Meta_Op<'+'> {
			template<class X, class Y>
			using apply = Idx<X::value + Y::value>;
		};
		template<> struct Meta_Op<'-'> {
			template<class X, class Y>
			using apply = Idx<X::value - Y::value>;
		};
		template<> struct Meta_Op<'*'> {
			template<class X, class Y>
			using apply = Idx<X::value* Y::value>;
		};
		template<> struct Meta_Op<'>'> {
			template<class X, class Y>
			struct apply { static const bool value = (X::value > Y::value); };
		};
		template<> struct Meta_Op<'='> {
			template<class X, class Y>
			struct apply { static const bool value = (X::value == Y::value); };
		};
		using plus = Meta_Op<'+'>;
		using sub = Meta_Op<'-'>;
		using mul = Meta_Op<'*'>;
		using greater = Meta_Op<'>'>;
		using equal = Meta_Op<'='>;
	}

	template<class ...L>
	using meta_expr = typename Meta_Expr<L...>::type;

	template<class ...L>
	using meta_expr_string = Meta_Expr<L...>;
}
namespace meta_traits
{
#define quick_invoke(tmp, alias) typename tmp::template alias
#define quick_apply(tmp) quick_invoke(tmp, apply)
#define no_typename_apply(tmp) tmp::template apply
#define quick_value(T) T::value
#define MMO(t) m_meta_object_##t

	using namespace exp_repeat;
	using namespace exp_repeat::operators;


	template<class T> struct quick_reverse
	{
		static constexpr bool value = (!T::value);
	};

	template<size_t N> struct quick_value_i
	{
		static constexpr size_t value = N;
	};

	template<bool b> struct quick_value_b
	{
		static constexpr bool value = b;
	};

	template<class T> constexpr bool quick_bool_value = T::value;


	template<size_t A, size_t B> struct quick_value_i_greater
	{
		static constexpr bool value = (A > B);
	};
	template<size_t A, size_t B> struct quick_value_i_greater_equal
	{
		static constexpr bool value = (A >= B);
	};
	template<size_t A, size_t B> struct quick_value_i_lesser
	{
		static constexpr bool value = (A < B);
	};
	template<size_t A, size_t B> struct quick_value_i_lesser_equal
	{
		static constexpr bool value = (A <= B);
	};

	template<class TL1, class TL2>
	struct quick_size_compare_greater : quick_value_i_greater<exp_size<TL1>, exp_size<TL2>> {};

	template<class TL1, class TL2>
	struct quick_size_compare_greater_equal : quick_value_i_greater_equal<exp_size<TL1>, exp_size<TL2>> {};

	template<class TL1, class TL2>
	struct quick_size_compare_lesser : quick_value_i_lesser<exp_size<TL1>, exp_size<TL2>> {};

	template<class TL1, class TL2>
	struct quick_size_compare_lesser_equal : quick_value_i_lesser_equal<exp_size<TL1>, exp_size<TL2>> {};



	template<class F, class TL>
	struct exp_fn_apply_impl {};

	template<class F, template<class ...> class TL, class ...L>
	struct exp_fn_apply_impl<F, TL<L...>>
	{
		using type = TL<meta_invoke<F, L>...>;
	};

	template<class F, class TL> using exp_fn_apply = typename exp_fn_apply_impl<F, TL>::type;

	struct meta_empty { static constexpr int value = 0; };
	struct meta_empty_fn { template<class T, class ...> using apply = T; };

	struct meta_break_signal :std::false_type {};

	struct meta_always_continue
	{
		template<class T> struct apply :std::false_type
		{};
	};

	template<class Fn, class DF> struct meta_break_if
	{
		template<class T> using apply = exp_if<meta_invoke<Fn, T>::value, meta_break_signal, DF>;
	};



	/*A meta obj is a bind of a meta_function and an obj, each time it is invoked, it update itself to a new type,
	use ::type to get the inner obj*/
	template<class MMO(Obj), class F/*Define how to Update an obj*/>
	struct meta_object
	{
		using type = MMO(Obj);
		template<class ...Arg>
		using apply = meta_object<meta_invoke<F, MMO(Obj), Arg...>, F>;

		template<class Outer_Obj>
		using meta_set = meta_object<Outer_Obj, F>;
	};

	template<class MMO(Obj), class F, class Ret>
	struct meta_ret_object
	{
		using ret = meta_invoke<Ret, MMO(Obj)>;
		using type = MMO(Obj);
		template<class ...Arg>
		using apply = meta_ret_object<meta_invoke<F, MMO(Obj), Arg...>, F, Ret>;

		template<class Outer_Obj>
		using meta_set = meta_ret_object<Outer_Obj, F, Ret>;
	};

	template<size_t times, class MMO(Obj), class F, class break_f = meta_always_continue>
	struct meta_timer_object
	{
		using timer = meta_invoke<meta_break_if<break_f, quick_value_i_greater<times, 0>>, MMO(Obj)>;

		using type = MMO(Obj);
		template<class ...Arg>
		using apply = meta_timer_object<times - 1, meta_invoke<F, MMO(Obj), Arg...>, F, break_f>;

		template<class Outer_Obj>
		using meta_set = meta_timer_object<times, Outer_Obj, F, break_f>;

		template<size_t reset_time>
		using reset = meta_timer_object<reset_time, MMO(Obj), F, break_f>;
	};

	template<class MMO(Obj), size_t N, class break_f> struct To_Timer {};
	template<class obj, class F, size_t N, class break_f> struct To_Timer<meta_object<obj, F>, N, break_f>
	{
		using type = meta_timer_object<N, obj, F, break_f>;
	};

	template<class MMO(Obj), size_t N, class break_f = meta_always_continue> using to_timer = typename To_Timer<MMO(Obj), N, break_f>::type;

	template<class mo, class F> struct Break_If {};
	template<size_t N, class MMO(Obj), class MO_F, class F> struct Break_If<meta_timer_object<N, MMO(Obj), MO_F>, F>
	{
		using type = meta_timer_object<N, MMO(Obj), MO_F, F>;
	};

	template<class MTO, class BF> using break_if = get_type<Break_If<MTO, BF>>;

	template<class MMO(From), class MMO(To)>
	struct meta_transfer {
		using type = typename MMO(To)::template meta_set<typename MMO(From)::type>;
	};

	//transfer timer if invoke to a meta_timer_oject
	template<size_t times, class obj, class F, class MMO(To), class B>
	struct meta_transfer<meta_timer_object<times, obj, F, B>, MMO(To)> {
		using type = typename MMO(To)::template meta_set<typename meta_timer_object<times, obj, F, B>::timer>;
	};

	//transfer returns if invoke to a meta_ret_object
	template<class Ret, class obj, class F, class MMO(To)>
	struct meta_transfer<meta_ret_object<obj, F, Ret>, MMO(To)> {
		using type = typename MMO(To)::template meta_set<typename meta_ret_object<obj, F, Ret>::ret>;
	};

	//the meta_object is itself a meta_function
	//if two meta_objects invoked, invoke the first object with type in second object
	template<class MMO(Obj1), class MMO(Obj2)>
	struct Meta_Object_Invoke { using type = meta_invoke< MMO(Obj1), typename MMO(Obj2)::type>; };

	//if invoke with meta_ret_object, invoke the first object with returns
	template<class MMO(Obj1), class Obj2, class F, class Ret>
	struct Meta_Object_Invoke<MMO(Obj1), meta_ret_object<Obj2, F, Ret>>
	{
		using type = meta_invoke<MMO(Obj1), typename meta_ret_object<Obj2, F, Ret>::ret>;
	};
	template<class MMO(Obj1), class MMO(Obj2)>
	using meta_object_invoke = typename Meta_Object_Invoke< MMO(Obj1), MMO(Obj2)>::type;




	//transfer obj between MO, Fn not included.
	template<class MMO(From), class MMO(To)>
	using meta_transfer_object = typename  meta_transfer<MMO(From), MMO(To)>::type;

	//use meta_object_invoke to invoke two meta objects if true
	//meta object must be invoked with another meta object being argument
	//if false, return original meta object
	template<bool con> struct invoke_object_if
	{
		template<bool, class MO1, class MO2> struct meta_o_branch
		{
			using type = MO1;
		};
		template<class MO1, class MO2> struct meta_o_branch<true, MO1, MO2>
		{
			using type = meta_object_invoke<MO1, MO2>;
		};

		template<class MO1, class MO2> using apply = typename meta_o_branch<con, MO1, MO2>::type;
	};

	//using meta_invoke to invoke a meta function with arguments if true
	//if false, return the invoked meta function itself
	//this is designed for meta_object, because if it returns the function itself
	//it returns the object with not being invoked
	template<bool con> struct invoke_meta_function_if
	{
		template<bool, class F, class ...Args> struct meta_function_branch
		{
			using type = F;
		};
		template<class F, class ...Args> struct meta_function_branch<true, F, Args...>
		{
			using type = meta_invoke<F, Args...>;
		};

		template<class F, class ...Args> using apply = typename meta_function_branch<con, F, Args...>::type;
	};




	//Note: looper returns a meta object, not the context itself
	//Note: with the macro MMO, it requires the template class to be a meta_object
	template<bool, class MMO(Condition), class MMO(Obj), class MMO(Generator) = meta_object<meta_empty, meta_empty_fn>> struct meta_looper
	{


		template<class ...Args> struct apply {

			//transfer current obj to  condition_obj to judge
			//transfer different context based on types of meta_object
			using _continue_t = typename meta_invoke<meta_transfer_object<MMO(Obj), MMO(Condition)>>::type;
			static const bool _continue_ = _continue_t::value;

			//invoke generator object if condition is true
			using generator_stage_o = typename invoke_meta_function_if<_continue_>::template apply<MMO(Generator), Args...>;

			//invoke Obj object if condition is true
			using result_stage_o = typename invoke_object_if<_continue_>::template apply<MMO(Obj), generator_stage_o>;

			//for debug
			using next_stage = meta_looper<_continue_, MMO(Condition), result_stage_o, generator_stage_o>;

			//recursively loop for result
			using track_apply_t = meta_invoke<invoke_meta_function_if<_continue_>, meta_looper<
				_continue_,
				MMO(Condition),
				result_stage_o,
				generator_stage_o
			>, Args...>;
			using type = typename track_apply_t::type;
		};
	};

	template<class Cond, class MO, class Generator> struct meta_looper<false, Cond, MO, Generator>
	{
		static constexpr bool _continue_ = false;
		using type = MO;
	};
	template<class MMO(Condition), class MMO(Obj), class MMO(Generator) = meta_object<meta_empty, meta_empty_fn>>
	using meta_looper_t = typename meta_invoke<meta_looper<true, MMO(Condition), MMO(Obj), MMO(Generator)>>::type;

	template<class MMO(Condition), class MMO(Obj), class MMO(Generator) = meta_object<meta_empty, meta_empty_fn>>
	using meta_looper_stage = meta_invoke<meta_looper<true, MMO(Condition), MMO(Obj), MMO(Generator)>>;


	//easy interface for template functor recursion
	//imagination of while constexpr
	template<class...> struct while_constexpr {};
	template<typename mo_cnd, typename mo_obj, typename mo_gen>
	struct while_constexpr<mo_cnd, mo_obj, mo_gen>
	{
		template<class MO> struct is_breakable :std::false_type {};

		template<size_t N, class Obj, class MO_F, class BF>
		struct is_breakable<meta_timer_object<N, Obj, MO_F, BF>> : std::true_type {};

		using current_looper = meta_invoke<meta_looper<true, mo_cnd, mo_obj, mo_gen>>;

		template<typename F, typename ...Args>
		inline constexpr void recursively_invoke(F&& f, Args &&...args) const noexcept
		{

			if constexpr (current_looper::_continue_)
			{
				f.template operator() < typename current_looper::result_stage_o::type > (std::forward<Args>(args)...);
				while_constexpr<mo_cnd, typename current_looper::result_stage_o, typename current_looper::generator_stage_o>
				{}.recursively_invoke(std::forward<F>(f), std::forward<Args>(args)...);

			}
		}

		//provide a transform function to automatically use while_constexpr transform
		template<typename F, typename TF, class O, typename ...Args>
		inline constexpr void recursively_transform_invoke(F&& f, TF&& tf, O&& o, Args&& ...args) const noexcept
		{
			if constexpr (current_looper::_continue_)
			{
				f.template operator() < typename current_looper::result_stage_o::type > (
					std::forward<O>(o),
					std::forward<Args>(args)...
					);
				while_constexpr<mo_cnd, typename current_looper::result_stage_o, typename current_looper::generator_stage_o>
				{}.recursively_transform_invoke(std::forward<F>(f), std::forward<TF>(tf), std::move(tf.template operator()(o)), std::forward<Args>(args)...);
			}
		}

		constexpr auto final_type() const noexcept-> typename current_looper::type
		{
			return {};
		}



		constexpr bool is_blocked() const noexcept
		{
			static_assert(is_breakable<mo_obj>::value, "Error:current meta_object is not a meta_timer_object! Only meta_timer_object is breakable!");

			return std::is_same_v<typename decltype(final_type())::timer, meta_break_signal>;
		}
	};

	template<class mo_cnd, class mo_obj>
	struct while_constexpr<mo_cnd, mo_obj> : while_constexpr<mo_cnd, mo_obj, meta_object<meta_empty, meta_empty_fn>>
	{};

	//common meta_object generator
	namespace common_object
	{
		struct append
		{
			template<class thisObj, class T> struct append_impl
			{
				using type = exp_list<thisObj, T>;
			};
			template<template<class...> class TL, class Outer_ty, class...Typs> struct append_impl <TL<Typs...>, Outer_ty>
			{
				using type = TL<Typs..., Outer_ty>;
			};
			template<class thisObj, class _2> using apply = typename append_impl<thisObj, _2>::type;
		};
		struct decreased
		{
			template<class TL, class...> using apply = typename split_first<TL>::rest;
		};
		template<size_t _limit> struct size_limiter
		{
			template<class TL, class...> struct apply
			{
				static const bool value = size_of_type_list<TL>::value < _limit;
			};
		};
		template<size_t _above> struct size_above
		{
			template<class TL> struct apply
			{
				static const bool value = (size_of_type_list<TL>::value > _above);
			};
		};
		struct auto_inc_gen
		{
			template<class _idx, class...> using apply = exp_repeat::inc_idx_t<_idx>;
		};
		struct timer_receiver {
			template<class timer, class...>
			struct apply :timer
			{};
		};
		struct decrease_ret
		{
			template<class T> struct first_type {
				using type = no_exist_type;
			};
			template<template<class...> class TL, class T, class...rest> struct first_type <TL<T, rest...>>
			{
				using type = T;
			};
			template<class TL> using apply = typename first_type<TL>::type;
		};
		template<class condition_f> struct fliter : append
		{
			template<class thisObj, class _2> using apply = exp_if<
				(meta_invoke<condition_f, thisObj, _2>::value),
				append::apply<thisObj, _2>,
				thisObj
			>;
		};
		struct replace_transform {
			template<class thisObj, class _2> using apply = _2;
		};
		struct stop_when_true
		{
			template<class this_tl, class ...> struct apply : quick_reverse<this_tl>
			{};
		};

		template<class F> struct stop_when_true_if
		{
			template<class this_tl, class...> struct apply : quick_value_b<meta_invoke<F, this_tl>::value>
			{};
		};
		template<size_t N> struct meta_value_limiter_i_f
		{
			template<class Val_t, class...> struct apply : quick_value_i_lesser<Val_t::value, N>
			{};
		};

		template<template<class ...> class apply_shape> struct meta_function_template_container {};

		template<class F, class En = void> struct is_meta_function_type : std::false_type {};

		template<class F> struct is_meta_function_type <F, std::void_t<meta_function_template_container<F::template apply>>> : std::true_type {};

		template<class F> constexpr bool is_meta_function_v = is_meta_function_type<F>::value;

		struct fold_apply_f
		{
			template<class ThisMo, class Fn> using apply = meta_invoke<invoke_meta_function_if<is_meta_function_v<Fn>>, Fn, ThisMo>;
		};

		struct node_forward_f
		{
			template<class this_type, class ...> using apply = typename this_type::next_type;
		};

		template<class Node> using node_forward_o = meta_object<Node, node_forward_f>;

		struct node_forward_condition
		{
			template<class this_type, class ...> struct apply
			{
				static constexpr bool value = this_type::has_next;
			};
		};
		template<class Cache, class T> struct decrease_cache
		{
			using cache = Cache;
			using type = no_exist_type;
		};

		template<class Cache, template<class ...> class TL, class first, class ...rest>
		struct decrease_cache<Cache, TL<first, rest...>>
		{
			using pop_front = decrease_cache<first, TL<rest...>>;
			using cache = Cache;
			using type = TL<first, rest...>;
		};
		struct decrease_cache_f
		{
			template<class this_tl, class ...> using apply = typename this_tl::pop_front;
		};
		struct decrease_cache_ret
		{
			template<class this_tl> using apply = typename this_tl::cache;
		};




		template<class TL> using meta_cache_ret_o = meta_ret_object<decrease_cache<meta_empty, TL>, decrease_cache_f, decrease_cache_ret>;

		template<size_t N> using meta_value_limiter_i_co = meta_object<Idx<0>, meta_value_limiter_i_f<N>>;

		//a mo to perform fold algorithm
		template<class T>
		using meta_fold_apply_o = meta_object<T, fold_apply_f>;

		//a mo replace that replaces itself in looper
		using meta_replace_o = meta_object<meta_empty, replace_transform>;

		//a timer version of replace_o
		template<size_t N> using meta_replace_to = meta_timer_object<N, meta_empty, replace_transform>;

		//a mo convert typelist to an appendable list in looper
		template<class TL> using meta_appendable_o = meta_object<TL, append>;

		//a mo convert typelist to an appendable list with filter in looper
		template<class TL, class fn> using meta_appendable_filter_o = meta_object<TL, fliter<fn>>;

		//a mo convert typelist to an decreasible list in looper
		template<class TL> using meta_decreasible_o = meta_object<TL, decreased>;

		//a mo decrease itself step by step, return what is decrease when invoked
		template<class TL> using meta_ret_decreasible_o = meta_ret_object<TL, decreased, decrease_ret>;

		//generate a condition mo which limit max size of typelist when invoked
		template<size_t N> using meta_length_limiter_o = meta_object<meta_empty, size_limiter<N>>;

		//generate a condition mo which limit min size of typelist when invoked
		template<size_t N> using meta_length_above_o = meta_object<meta_empty, size_above<N>>;

		//a generator mo which produces a greater idx when invoked
		template<size_t N> using meta_idx_inc_gen_o = exp_if<
			N != 0,
			meta_object<exp_repeat::Idx<N - 1>, auto_inc_gen>,
			meta_object<exp_repeat::below_zero, auto_inc_gen>
		>;

		//generate a conditional type mo
		template<class Fn> using meta_condition_c_o = meta_object<meta_empty, Fn>;

		//empty mo, mo container
		using meta_empty_o = meta_object<meta_empty, meta_empty_fn>;

		//common timer condition mo 
		using meta_timer_cnd_o = meta_object<meta_empty, timer_receiver>;

		//a list for decrease mo incase of an empty list
		template<class ...Typs> using decrease_list = exp_list<meta_empty, Typs...>;

		template<class TL> using decrease_transform = quick_invoke(get_type<to_exp_list<TL>>, to) < decrease_list > ;


		//a condition mo the determin if a exp_node object is nextable
		using node_forward_condition_o = meta_condition_c_o<node_forward_condition>;
	}
	template<class MMO(Obj), class MMO(Generator) = common_object::meta_empty_o>
	using meta_timer_looper_t = meta_looper_t<common_object::meta_timer_cnd_o, MMO(Obj), MMO(Generator)>;
	template<class ...MO> using make_loop = meta_looper<true, MO...>;
	template<class MTO, class GO = common_object::meta_empty_o> using make_timer_loop = meta_looper<true, common_object::meta_timer_cnd_o, MTO, GO>;


	template<size_t N, class mo_obj, class mo_f, class break_f>
	struct while_constexpr<meta_timer_object<N, mo_obj, mo_f, break_f>> :
		while_constexpr<common_object::meta_timer_cnd_o, meta_timer_object<N, mo_obj, mo_f, break_f>>
	{};

	template<class _del_idx, class _ins_idx, class TL>
	struct delete_and_insert_to
	{
		template<class _idx> struct nth_of
		{
			static_assert(_idx::value <= max_type_list_index<TL>::value, "idx overflow!!!");
			template<class ITL> using apply = exp_select<_idx::value, ITL>;
		};
		template<class _idx> struct del_at
		{
			static_assert(_idx::value <= max_type_list_index<TL>::value, "idx overflow!!!");
			template<class ITL> using apply = typename experiment::erase_at_t<_idx::value, ITL>::type;
		};
		template<class _idx, class T> struct ins_at
		{
			static_assert(_idx::value <= max_type_list_index<TL>::value, "idx overflow!!!");
			template<class ITL> using apply = insert_at<ITL, inserter<_idx::value, T>>;
		};
		using recv = meta_object<meta_empty, nth_of<_del_idx>>;
		using delr = meta_object<TL, del_at<_del_idx>>;
		using recv_inv = meta_transfer_object<delr, recv>;
		using delr_t = typename meta_invoke<delr>::type;
		using recv_t = typename meta_invoke<recv_inv>::type;
		using type = meta_invoke<ins_at<_ins_idx, recv_t>, delr_t>;
	};

	template<class idx_start, class idx_end, class TL> struct meta_swap
	{
		static const bool start_is_greater = greater::template apply<idx_start, idx_end>::value;
		using stage_t = delete_and_insert_to<idx_start, idx_end, TL>::type;
		using type = delete_and_insert_to<
			exp_if<start_is_greater, inc_idx_t<idx_end>, meta_expr<idx_end, sub, exp_repeat::Idx<1>>>,
			idx_start, stage_t>::type;
	};
	template<class I> struct meta_integer_segment {};
	template<size_t I> struct meta_integer_segment<exp_repeat::Idx<I>>
	{
		template<class T> struct count_impl {};
		template<size_t counts> requires (counts >= 1) struct count_impl<Idx<counts>>
		{
			using count_gen = typename meta_timer_looper_t<
				meta_timer_object<counts, exp_list<>, common_object::append>,
				common_object::meta_idx_inc_gen_o<I>
			>::type;
			using type = count_gen;
		};
		template<class counts>
		using apply = get_type<count_impl<counts>>;
	};

	template<size_t I, size_t counts> requires (counts >= 1)
		using meta_count = meta_integer_segment<Idx<I>>::template apply<Idx<counts>>;

	template<size_t I, size_t counts> using list_slice = exp_repeat::meta_to_array<
		meta_count<I, counts>>::template to<exp_select_list>;
}
template<std::size_t N, class Obj, class F, class BF> 
struct type_list_size<meta_traits::meta_timer_object<N, Obj, F, BF>>
{
    static constexpr std::size_t value = N;
};


namespace meta_typelist
{
	using namespace meta_traits;
	using namespace meta_traits::common_object;

	template<class TL>
	struct meta_clear {};
	template<template<class...> class TL, class...L>
	struct meta_clear<TL<L...>> { using type = TL<>; };
	template<class TL> using meta_clear_t = typename meta_clear<TL>::type;


	//define meta_stream object
	//meta stream contains an input stream (From) and an output stream (To)
	template<class MMO(To), class MMO(From)> struct meta_stream
	{
		using from = MMO(From);
		using to = MMO(To);
		using cache = typename MMO(From)::ret;
		using update = meta_stream<
			typename invoke_object_if<(exp_size<typename MMO(From)::type> > 0)>
			::template apply<MMO(To), MMO(From)>, meta_invoke<MMO(From)>
		>;
	};
	struct meta_stream_f
	{
		template<class mo_stream, class...>
		using apply = typename mo_stream::update;
	};

	//common output stream
	template<class TL>
	using meta_ostream = meta_appendable_o<TL>;

	//common input stream
	template<class TL>
	using meta_istream = meta_ret_decreasible_o<TL>;

	//a stream that generate increasing integer
	template<size_t N> using meta_integer_istream = meta_ret_decreasible_o<typename meta_iota<N>::template to<decrease_list>>;

	//the meta_stream_object is based on meta_timer_object
	//add a break condition if you try to block the stream
	template<size_t Transfer_Length, class MMO(To), class MMO(From), class break_f = meta_always_continue>
	//requires (size_of_type_list<typename MMO(From)::type>::value >= Transfer_Length)
	using meta_stream_o = meta_timer_object<
		Transfer_Length,
		meta_stream< MMO(To), MMO(From)>,
		meta_stream_f,
		break_f
	>;
	//do all transfer between two meta_object
	//add a break condition if you try to block the stream
	template<class MMO(To), class MMO(From), class break_f = meta_always_continue>
	using meta_all_transfer = typename meta_timer_looper_t<
		meta_stream_o<size_of_type_list<typename MMO(From)::type>::value, MMO(To), MMO(From), break_f>
	>::type;

	//a stream that will tranfer all types in From to To 
	template<class MMO(To), class MMO(From), class break_f = meta_always_continue> using meta_all_transfer_o = meta_stream_o<size_of_type_list<typename MMO(From)::type>::value, MMO(To), MMO(From), break_f>;



	template<class MMO(stream)> struct Meta_Stream_Transfer {};
	template<size_t Transfer_Length, class MMO(To), class MMO(From)>
	struct Meta_Stream_Transfer<meta_stream_o<Transfer_Length, MMO(To), MMO(From) >>
	{
		using timer = meta_timer_looper_t<meta_stream_o<Transfer_Length, MMO(To), MMO(From)>>;
		using mo = typename meta_timer_looper_t<meta_stream_o<Transfer_Length, MMO(To), MMO(From)>>::type;
		using type = typename meta_timer_looper_t<meta_stream_o<Transfer_Length, MMO(To), MMO(From)>>::type::to::type;
	};

	template<class MMO(stream)> using meta_stream_transfer = typename Meta_Stream_Transfer<MMO(stream)>::type;
	template<class MMO(stream)> using meta_stream_transfer_mo = typename Meta_Stream_Transfer<MMO(stream)>::mo;
	template<class MMO(stream)> using meta_stream_transfer_timer = typename Meta_Stream_Transfer<MMO(stream)>::timer;

	template<class MMO(O)> using clear_meta_oject = typename MMO(O)
		:: template meta_set<meta_clear_t<typename MMO(O)::type>>;

	template<class mso, std::size_t N = 0> struct clear_stream {};

	template<auto A, auto B> constexpr auto max_value = (A > B) ? A : B;
	template<auto A, auto B> constexpr auto min_value = (A < B) ? A : B;

	template<std::size_t N, class MMO(TO), class MMO(From), std::size_t Left>
	struct clear_stream<meta_stream_o<Left, MMO(TO), MMO(From)>, N>
	{
		using clear_os = clear_stream<meta_stream_o<Left, clear_meta_oject<MMO(TO)>, MMO(From)>, N>;
		using clear_is = clear_stream<meta_stream_o<Left, MMO(TO), clear_meta_oject<MMO(From)>>, N>;
		using reset_time = clear_stream<meta_stream_o<N, MMO(TO), MMO(From)>, N>;
		using type = meta_stream_o<Left, MMO(TO), MMO(From)>;
	};


	struct meta_linker
	{
		template<class MMO(op), class MMO(ip), class F> struct apply
		{
			using op = meta_invoke<MMO(op), meta_invoke<F, typename MMO(ip)::ret>>;
			using ip = meta_invoke<MMO(ip)>;
			using type = typename meta_all_transfer<op, ip>::to::type;
		};
	};

	template<class ...Typs>
	struct selectable_list;
	

	template<size_t N> struct meta_spliter
	{
		template<class TL>
		using transfer_s = typename meta_timer_looper_t<
			meta_stream_o<N, meta_ostream<meta_clear_t<TL>>, meta_istream<TL>>
		>::type;
		template<class TL>
		using back = typename transfer_s<TL>::from::type;
		template<class TL>
		using front = typename transfer_s<TL>::to::type;
		template<class TL, class F>
		using apply = meta_linker::apply<typename transfer_s<TL>::to, typename transfer_s<TL>::from, F>;
	};
	template<size_t N> struct meta_transform_at
	{
		template<class TL, class F> using apply = meta_spliter<N>::template transform<TL, F>;
	};


	template<size_t N, class T> struct meta_tag {
		using type = T;
		static const size_t value = N;
	};

	namespace alias_c
	{
		template<template<class> class F> constexpr size_t alias_argc() { return 1; }
		template<template<class, class> class F> constexpr size_t alias_argc() { return 2; }
		template<template<class, class, class> class F> constexpr size_t alias_argc() { return 3; }
		template<template<class, class, class, class> class F> constexpr size_t alias_argc() { return 4; }
		template<template<class, class, class, class, class> class F> constexpr size_t alias_argc() { return 5; }
		template<template<class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 6; }
		template<template<class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 7; }
		template<template<class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 8; }
		template<template<class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 9; }
		template<template<class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 10; }
		template<template<class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 11; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 12; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 13; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 14; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 15; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 16; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 17; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 18; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 19; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 20; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 21; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 22; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 23; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 24; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 25; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 26; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 27; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 28; }

		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 29; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 30; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 31; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 32; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 33; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 34; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 35; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 36; }
		template<template<class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class, class> class F> constexpr size_t alias_argc() { return 37; }

		template<template<class...> class F, class ...Typs> struct alias_invoke
		{
			using type = F<Typs...>;
		};

	}


	//quickly create quoted-meta-functions from the unquoted ones
	namespace quick_meta
	{
		using namespace alias_c;

		template<template<class ...> class al_f, class ...Args> struct invoke_alias
		{
			//this invoker calculate the needed arguments from the alias function and automatically apply it
			using real_args_list_so = meta_stream_o<alias_argc<al_f>(), meta_ostream<exp_list<>>, meta_istream<exp_list<Args...>>>;
			using type = quick_invoke(meta_stream_transfer<real_args_list_so>, to) < al_f > ;
		};

		template<template<class...> class unquoted_meta_fn> struct quick_construct
		{
			template<class ...Args> using apply = get_type<invoke_alias<unquoted_meta_fn, Args...>>;
		};

		template<class T> struct quick_nested
		{
			template<class Unused = void> using apply = T;
		};

		template<template<class...> class unquoted_meta_fn, class ...Bounded_Fn_Args> struct quick_construct_bind
		{
			template<class ...Args> using apply = get_type<invoke_alias<unquoted_meta_fn, Bounded_Fn_Args..., Args...>>;
		};

		template<template<class...> class unquoted_meta_fn, template<class ...> class args_transform, class ...Bounded_Fn_Args>
		struct quick_construct_bind_transform_args
		{
			template<class ...Args> using apply = get_type<invoke_alias<unquoted_meta_fn, Bounded_Fn_Args..., args_transform<Args>...>>;;
		};

		template<template<class> class F> struct quick_meta_object_function_single_to_object
		{
			template<class _1, class ...> using apply = F<_1>;
		};

		template<template<class, class> class F> struct quick_meta_object_function
		{
			template<class _1, class _2> struct apply : F<_1, _2> {};
		};

		template<template<class, class> class F, class _2> struct quick_meta_object_function_bind
		{
			template<class this_obj, class ...> struct apply : F<this_obj, _2>
			{};
		};

		template<template<class, class> class F, class T> struct quick_meta_object_function_ignored_this
		{
			template<class this_obj, class mo_arg> struct apply :F<T, mo_arg> {};
		};


	}

	template<class T> struct stream_final_type { using type = T; };
	template<class mso> requires requires{typename mso::to::type; }
	struct stream_final_type<mso> {
		using type = typename mso::to::type;
	};

	template<class mso> using stream_final_t = typename stream_final_type<mso>::type;

	template<class F> struct meta_stream_op
	{
		template<class mso, class ...Args> using apply = meta_invoke<F, stream_final_t<mso>, Args...>;
	};

	template<class ...Typs>
	struct selectable_list : exp_list<Typs...>
	{
		template<class replace_type> struct replace_impl
		{
			template<class T> using apply = replace_type;
		};
		struct replace_obj
		{
			template<class thisObj, class T> using apply = T;
		};
		template<size_t N> using replace_this_o = meta_timer_object<N, meta_empty_o, replace_obj>;

		template<size_t N> using get = typename meta_looper_t<
			meta_timer_cnd_o,
			replace_this_o<N + 1>,
			meta_ret_decreasible_o<decrease_list<Typs...>>>::type;

		template<size_t N>
		struct _tag {
			template<class T> using apply = meta_tag<N, T>;
		};

		template<size_t N> struct invoke
		{
			template<class T>
			using replace_with = typename meta_spliter<N>::template apply<selectable_list<Typs...>, replace_impl<T>>::type;

			template<class F>
			using transform_to = typename meta_spliter<N>::template apply<selectable_list<Typs...>, F>::type;

			using tag = transform_to<_tag<N>>;

			template<size_t NS> using tag_to = transform_to<_tag<NS>>;


			template<size_t idx>
			using swap_with = typename meta_swap<exp_repeat::Idx<N>, exp_repeat::Idx<idx>, selectable_list<Typs...>>::type;

			template<class T>
			using insert = insert_at<selectable_list<Typs...>, inserter<N, T>>;
		};
	};

	template<> struct selectable_list<> {};

	template<class T> struct to_selectable { using type = selectable_list<T>; };

	template<template<class...> class TL, class ...Typs>
	struct to_selectable<TL<Typs...>> { using type = selectable_list<Typs...>; };

	template<template<class...> class TL>
	struct to_selectable<TL<>> { using type = selectable_list<>; };

	template<class TL>
	using to_selectable_t = typename to_selectable<TL>::type;

	template<class TL> struct meta_rename {};
	template<template<class...> class TL, class ...T> struct meta_rename<TL<T...>>
	{
		template<template<class...> class TL2>
		using with = TL2<T...>;
	};
	template<class TL, class MArr> struct meta_swap2_type
	{
		using iter = to_selectable_t<TL>;
		static constexpr size_t pos1 = MArr::template at<0>::value;
		static constexpr size_t pos2 = MArr::template at<1>::value;

		using first_transformed = typename iter::template invoke<pos1>::template replace_with<exp_select<pos2, TL>>;
		using type = typename first_transformed::template invoke<pos2>::template replace_with<exp_select<pos1, TL>>;

	};

	namespace list_common_object
	{
		template<class TL1, class TL2> struct append_list;
		template<template<typename...> typename tlist1, typename ...Typs1, template<typename ...> typename tlist2, typename ...Typs2>
		struct append_list<tlist1<Typs1...>, tlist2<Typs2...>>
		{
			using type = tlist1<Typs1..., Typs2...>;
		};

		template<class front, class back> struct trim_back { using type = no_exist_type; };
		template<
			template<class...> class front,
			template<class...> class back,
			class ...front_types,
			class back_first, class ...back_rest>
		struct trim_back<front<front_types...>, back<back_first, back_rest...>>
		{
			using type = typename trim_back<front<front_types..., back_first>, back<back_rest...>>::type;
		};
		template<template<class ...> class front,
			template<class> class back,
			class ...front_types,
			class back_type
		>
		struct trim_back<front<front_types...>, back<back_type>>
		{
			using type = front<front_types...>;
		};

		template<class T> struct trim;
		template<template<class...> class tlist, class ...types> struct trim<tlist<types...>>
		{
			using type = typename trim_back<tlist<>, tlist<types...>>::type;
		};

		template<class T> struct trim_f;
		template<template<class...> class tlist, class first, class ...rest> struct trim_f<tlist<first, rest...>>
		{
			using type = tlist<rest...>;
		};

		template<class...Args> struct meta_list :exp_list<Args...>
		{
			static constexpr std::size_t length = sizeof...(Args);

			template<template<typename ...> typename m_alias>
			using as = m_alias<Args...>;

			template<class T> using append = meta_list<Args..., T>;
			template<class T> using r_append = meta_list<T, Args...>;

			template<class T = void>
			using trim_last = get_type<trim<meta_list<Args...>>>;

			template<class T = void>
			using trim_front = get_type<trim_f<meta_list<Args...>>>;

		};
		//from -> to
		struct reverse_decrease
		{
			template<class meta_tl> struct apply_impl
			{
				using type = no_exist_type;

			};

			template<template<class...> class this_tl, class first, class ...types> struct apply_impl<this_tl<first, types...>>
			{
				using type = typename meta_list<first, types...>::template trim_last<>::template to<this_tl>;
			};

			template<class this_tl, class ...> using apply = get_type<apply_impl<this_tl>>;
		};

		struct reverse_decrease_ret
		{
			template<class this_tl> struct apply_impl
			{
				using type = no_exist_type;
			};
			template<template<class...> class this_tl, class first, class ...types> struct apply_impl<this_tl<first, types...>>
			{
				using type = exp_select<
					max_index<this_tl<first, types...>>,
					this_tl<first, types...>
				>;
			};

			template<class this_tl> using apply = get_type<apply_impl<this_tl>>;
		};

		template<class TL> using meta_reverse_decrease_o = meta_object<TL, reverse_decrease>;
		template<class T> using meta_empty_alias = T;
		template<class T> using meta_self_o = meta_object<T, meta_empty_fn>;

		template<template<class> class F>
		struct list_apply_f
		{
			template<class this_tl, class T> using apply = exp_fn_apply<F<T>, this_tl>;
		};

		struct attach_f
		{
			template<class this_selectable_o, class T> using apply = selectable_list<typename this_selectable_o::template get<0>, T>;
		};
		template<std::size_t N>
		struct mso_stride
		{
			template<class source_empty_mso>
			struct impl
			{
				using type = source_empty_mso;
			};

			template<class mso> requires (exp_size<typename mso::type::to::type> > 0)
				struct impl<mso>
			{
				using reset_timer_t = typename mso::template reset<N>;
				using clear_os_t = typename clear_stream<reset_timer_t>::clear_os::type;
				using type = meta_stream_transfer_timer<clear_os_t>::template reset<N>;
			};
			template<class this_mso, class ...>
			using apply = typename impl<this_mso>::type;
		};

		template<std::size_t N>
		struct mso_stride_ret
		{
			template<class this_mso> using apply = typename this_mso::type::to::type;
		};


		//interface
		template<std::size_t Stride, class TL> using meta_stride_istream = meta_ret_object<
				meta_stream_o<(exp_size<TL> / Stride),
				meta_ostream<typename meta_spliter<Stride>::template front<TL>>,
				meta_istream<typename meta_spliter<Stride>::template back<TL>>
			>, mso_stride<Stride>, mso_stride_ret<Stride>
		>;
		template<class TL, template<class> class F> using meta_list_apply_o = meta_object<TL, list_apply_f<F>>;
		template<std::size_t N, class TL, template<class> class F> using meta_list_apply_to = meta_timer_object<N, TL, list_apply_f<F>>;
		template<class T> using meta_attach_o = meta_object<selectable_list<T, T>, attach_f>;
		template<class TL> using meta_reverse_istream = meta_ret_object<TL, reverse_decrease, reverse_decrease_ret>;
				template<class TL> struct reverse_typelist
		{
			using type = typename meta_all_transfer<
				meta_ostream<exp_list<>>,
				meta_reverse_istream<TL>
			>::to::type;
		};
		template<class TL> using reverse_t = typename reverse_typelist<TL>::type;

	}

	namespace collector
	{
		template<class looper> constexpr bool meta_looper_status =
			meta_invoke<invoke_meta_function_if<is_meta_function_v<looper>>, looper>::_continue_;

		//break_condition
		struct collect_stream_break_if_looper_stop
		{
			template<class collect_stream> struct apply
			{
				static constexpr bool value = !meta_looper_status<typename collect_stream::from::type>;
			};
		};

		//progressing the looper, use the looper itself as a meta istream
		struct collect_stream_update_f
		{
			template<class this_looper> struct impl
			{
				using type = no_exist_type;
			};
			template<bool c, class Mc, class Mo, class Mg> struct impl<meta_looper<c, Mc, Mo, Mg>>
			{
				using l_invoke_t = meta_invoke<meta_looper<c, Mc, Mo, Mg>>;
				using type = meta_looper<c, Mc,
					typename l_invoke_t::result_stage_o, typename l_invoke_t::generator_stage_o>;
			};
			template<class this_looper, class ...> using apply = typename impl<this_looper>::type;
		};

		struct collect_stream_ret_f
		{
			template<class this_looper> using apply = typename meta_invoke<this_looper>::result_stage_o::type;
		};

		//instantiate the type_list_size with looper type to make the looper size infinitely long
		//because we can't predict what is processing in the looper, we use a break condition instead to end the meta-stream


		//the meta-stream object can be use in a while_constexpr or meta_looper directly
		template<class looper, class TL = exp_list<>> using meta_collect_ostream = break_if<meta_stream_o<
			exp_size<looper>,
			meta_ostream<TL>,
			meta_ret_object<looper, collect_stream_update_f, collect_stream_ret_f>
		>, collect_stream_break_if_looper_stop>;

		template<class looper, class TL = exp_list<>> struct looper_collector
		{
			using type = typename meta_timer_looper_t<meta_collect_ostream<looper, TL>>::type::to::type;
		};


		template<std::size_t N, class os, class ins, class BF, class TL>
		struct looper_collector<meta_stream_o<N, os, ins, BF>, TL> :
			exp_apply<typename collector::looper_collector<make_timer_loop<meta_stream_o<N, os, ins, BF>>, TL>::type, stream_final_t>
		{};

		template<class looper, class TL = exp_list<>> using collect = typename looper_collector<looper, TL>::type;



		template<class T, template<class> class ...collectable_f> struct meta_pipe_collect_stream_impl
		{
			template<class obj, template<class ...> class ...collectable_> struct collect_apply_recurse;
			template<class obj, template<class...> class first, template<class...> class ...rest> struct collect_apply_recurse<obj, first, rest...>
			{
				using recurse_type = collect_apply_recurse<collect<first<obj>>, rest...>;
				using type = typename recurse_type::type;
			};
			template<class obj> struct collect_apply_recurse<obj>
			{
				using type = obj;
			};
			using type = typename collect_apply_recurse<T, collectable_f...>::type;
		};

		template<class T, template<class> class...collectable_f> using meta_pipe = typename meta_pipe_collect_stream_impl<T, collectable_f...>::type;
	}
	template<class looper, class F, class ...Args>
	constexpr std::size_t collect_looper(F&& f, Args&& ...args)
	{
		using used_types = collector::collect<looper>;
		return template_func_execute_launcher(used_types{}, f, std::forward<Args>(args)...);
	}

}

template<class...Typs> struct type_list_size<exp_list<meta_traits::meta_empty, Typs...>>
{
	static constexpr size_t value = sizeof...(Typs);
};
template<class cache, class TL> struct type_list_size<meta_traits::common_object::decrease_cache<cache, TL>> :size_of_type_list<TL> {};
template<bool c, class Mc, class Mo, class Mg>
struct type_list_size<meta_traits::meta_looper<c, Mc, Mo, Mg>> :meta_traits::quick_value_i<static_cast<std::size_t>(-1)> {};

