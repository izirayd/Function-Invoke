#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <utility>
#include <functional>
#include <Windows.h>
#include <type_traits>
#include <utility>
#include <typeinfo>
#include <string>
#include <cassert>
#include <unordered_map>

template<size_t N = 16>
class store {
	char space[N];

	template<typename T>
	static constexpr bool
		fits() { return sizeof(typename std::decay<T>::type) <= N; }
public:
	template<typename D, typename V>
	D *copy(V &&v)	{
		return fits<D>() ? new(space) D{ std::forward<V>(v) } :
			new        D{ std::forward<V>(v) };
	}
	template<typename D, typename V, typename B>
	B *move(V &&v, B *&p)	{
		B *q = fits<D>() ? copy<D>(std::forward<V>(v)) : p;
		p = nullptr;
		return q;
	}
	template<typename D>
	void free(D *p) { fits<D>() ? p->~D() : delete p; }
};

template<typename A = store<>>
class some : A {
	using id = size_t;
	template<typename T>
	struct type { static void id() { } };
	template<typename T>
	static id type_id() { return reinterpret_cast<id>(&type<T>::id); }
	template<typename T>
	using decay = typename std::decay<T>::type;
	template<typename T>
	using none = typename std::enable_if<!std::is_same<some, T>::value>::type;
	struct base	{
		virtual ~base() { }
		virtual bool is(id) const = 0;
		virtual base *copy(A&) const = 0;
		virtual base *move(A&, base*&) = 0;
		virtual void free(A&) = 0;
	} *p = nullptr;

	template<typename T>
	struct data : base, std::tuple<T> 	{
		using std::tuple<T>::tuple;
		T       &get()      & { return std::get<0>(*this); }
		T const &get() const& { return std::get<0>(*this); }
		bool is(id i) const override        { return i == type_id<T>(); }
		base *copy(A &a) const override	    { return a.template copy<data>(get()); 	}
		base *move(A &a, base *&p) override	{ return a.template move<data>(std::move(get()), p); }
		void free(A &a) override { a.free(this); }
	};

	template<typename T>
	T &stat() { return static_cast<data<T>*>(p)->get(); }
	template<typename T>
	T const &stat() const { return static_cast<data<T> const*>(p)->get(); }
	template<typename T>
	T &dyn() { return dynamic_cast<data<T>&>(*p).get(); }
	template<typename T>
	T const &dyn() const { return dynamic_cast<data<T> const&>(*p).get(); }

	base *move(some &s) { return s.p->move(*this, s.p); }
	base *copy(some const &s) { return s.p->copy(*this); }
	base *read(some &&s) { return s.p ? move(s) : s.p; }
	base *read(some const &s) { return s.p ? copy(s) : s.p; }

	template<typename V, typename U = decay<V>, typename = none<U>>
	base *read(V &&v) { return A::template copy<data<U>>(std::forward<V>(v)); }

	template<typename X>
	some &assign(X &&x)	{
		if (!p) p = read(std::forward<X>(x));
		else	{
			some t{ std::move(*this) };
			try { p = read(std::forward<X>(x)); }
			catch (...) { p = move(t); throw; }
		}
		return *this;
	}

	void swap(some &s)	{
		if (!p)   p = read(std::move(s));
		else if (!s.p) s.p = move(*this);
		else		{
			some t{ std::move(*this) };
			try { p = move(s); }
			catch (...) { p = move(t); throw; }
			s.p = move(t);
		}
	}

public:
	some() { }
	~some() { if (p) p->free(*this); }

	some(some &&s) : p{ read(std::move(s)) } { }
	some(some const &s) : p{ read(s) } { }

	template<typename V, typename = none<decay<V>>>
	some(V &&v) : p{ read(std::forward<V>(v)) } { }

	some &operator=(some &&s) { return assign(std::move(s)); }
	some &operator=(some const &s) { return assign(s); }

	template<typename V, typename = none<decay<V>>>
	some &operator=(V &&v) { return assign(std::forward<V>(v)); }

	friend void swap(some &s, some &r) { s.swap(r); }

	void clear() { if (p) { p->free(*this); p = nullptr; } }

	bool empty() const { return p; }

	bool exist() {
		if (p == nullptr) return false;
		return true;
	}

	template<typename T>
	bool is() const { return p ? p->is(type_id<T>()) : false; }

	template<typename T> T      &&_() && { return std::move(stat<T>()); }
	template<typename T> T       &_()      & { return stat<T>(); }
	template<typename T> T const &_() const& { return stat<T>(); }

	template<typename T> T      &&cast() && { return std::move(dyn<T>()); }
	template<typename T> T       &cast()      & { return dyn<T>(); }
	template<typename T> T const &cast() const& { return dyn<T>(); }

	template<typename T> operator T && () && { return std::move(_<T>()); }
	template<typename T> operator T      &()      & { return _<T>(); }
	template<typename T> operator T const&() const& { return _<T>(); }
};

namespace std {	using any = some<>; }

template<typename _Func>
struct fun_signature_t;

template<typename _Ret, typename ..._Args>
struct fun_signature_t<_Ret(*)(_Args...)> {
	using type = std::function<void(_Args...)>;
};

template<typename _Class, typename _Ret, typename ..._Args>
struct fun_signature_t<_Ret(_Class::*)(_Args...)> {
	using type = std::function<void(_Args...)>;
};

template<typename _Class, typename _Ret, typename ..._Args>
struct fun_signature_t<_Ret(_Class::*)(_Args...) const> {
	using type = std::function<void(_Args...)>;
};

struct fun_signature {
	template <typename _Obj, typename _Type = decltype(&_Obj::operator()) >
	static auto get_signature(_Obj &, _Type = nullptr)->_Type;

	template< typename _Obj, typename _Type = _Obj * >
	static auto get_signature(_Obj * = {})->_Type;
};

class FunctionInvoke
{
public:
	FunctionInvoke() = default;
	~FunctionInvoke() = default;

	template<typename _FObj, typename ..._Args>
	void AddFunction(const std::string & name, const _FObj & fun)	{
		using f_type = decltype(fun_signature::get_signature(fun));
		using func = typename fun_signature_t<f_type>::type;
		std::cout << "\nf_type: " << typeid(f_type).name();
		std::cout << "\nfunc: " << typeid(func).name();
		_mapped_fun.emplace(name, func(fun));
	}

	template<typename ..._Args>
	void Invoke(const std::string & method, _Args ... args) 	{
		auto fun_any = _mapped_fun[method];
		if (fun_any.exist())	{
			using signature = std::function<void(_Args...)>;


			std::cout << "\nsignature: " << typeid(signature).name();
			auto func = fun_any._<signature>();
			std::string type_name1 = typeid(signature).name();
			std::string type_name2 = typeid(fun_any).name();
			if (fun_any.is<signature>() || type_name1 == type_name2)
				func(args...);
			else
				std::cout << "\nwrong sign: " << typeid(signature).name() << "\nexpected  : " << typeid(fun_any).name() << std::endl;
		}
	}

private:
	std::unordered_map< std::string, std::any > _mapped_fun;
};

namespace std { using function_invoke = FunctionInvoke; }