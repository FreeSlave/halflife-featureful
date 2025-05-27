#pragma once
#ifndef OPTIONAL_H
#define OPTIONAL_H

#include <cassert>
#include <new>
#include <utility>
#include <type_traits>

template< typename T > struct optional
{
	static_assert(!std::is_reference< T >::value,
	"reference is not supported as optional value");
	typedef optional me;

	// constructors
	optional() : f(false) {}
	optional(me const& rhs): f(rhs.f) { create(rhs); }
	optional(me&& rhs): f(rhs.f) { create(std::forward< me >(rhs)); }
	explicit optional(T const& w): f(true) { create(w); }

	// destructor
	~optional() { if(f) destroy(); }

	// check for value presence
	bool operator ! () const { return !f; }
	constexpr explicit operator bool() const noexcept { return f; }
	constexpr bool has_value() const noexcept { return f; }

	// assign
	me& operator = (me const& rhs) { return assign(rhs); }
	me& operator = (me&& rhs) { return assign(rhs); }
	me& operator = (T const& w) { return assign(w); }

	// get value
	T const& operator * () const { return *get(); }
	T& operator * () { return *get(); }
	T const* operator -> () const { return get(); }
	T* operator -> () { return get(); }

	template< class U >
	constexpr T value_or( U&& default_value ) const&
	{
		return f ? *get() : static_cast<T>(std::forward<U>(default_value));
	}
private:
	void create(T const& w) {
		new(v) T(w);
	}

	void create(T&& w) {
		new(v) T(std::forward< T >(w));
	}

	void create(me const& rhs) {
		if(f) create(*rhs.get());
	}

	void create(me&& rhs) {
		if(f) create(std::move(*rhs.get()));
	}

	void destroy() {
		get()->~T();
	}

	T const* get() const {
		assert(f && "no optional value");
		return reinterpret_cast< const T* >(v);
	}

	T* get() {
		assert(f && "no optional value");
		return reinterpret_cast< T* >(v);
	}

	void cleanup() {
		destroy(); f = false;
	}

	me& assign(T const& w) {
		if(f) *get() = w;
		else create(w), f = true;
		return *this;
	}

	me& assign(T&& w) {
		if(f) *get() = w;
		else create(std::forward(w)), f = true;
		return *this;
	}

	me& assign(me const& rhs) {
		if(rhs.f) return assign(*rhs.get());
		if(!f) return *this;
		cleanup();
		return *this;
	}

	me& assign(me&& rhs) {
		if(rhs.f) return assign(std::move(*rhs.get()));
		if(!f) return *this;
		cleanup();
		return *this;
	}

	bool f;
	alignas(T) char v[sizeof(T)];
};

#endif
