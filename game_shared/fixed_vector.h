#ifndef FIXED_ARRAY_H
#define FIXED_ARRAY_H

#include <array>
#include <cstddef>
#include <initializer_list>
#include <utility>
#include "min_and_max.h"

template<typename T, size_t N>
struct fixed_vector
{
private:
	typedef std::array<T, N> array_type;
public:
	typedef typename array_type::value_type value_type;
	typedef typename array_type::size_type size_type;
	typedef typename array_type::difference_type difference_type;
	typedef typename array_type::reference reference;
	typedef typename array_type::const_reference const_reference;
	typedef typename array_type::pointer pointer;
	typedef typename array_type::const_pointer const_pointer;
	typedef typename array_type::iterator iterator;
	typedef typename array_type::const_iterator const_iterator;

	fixed_vector(): _count(0) {}
	template<typename U>
	fixed_vector(std::initializer_list<U> list)
	{
		setList(std::move(list));
	}
	template<size_t M>
	fixed_vector(const fixed_vector<T, M>& o)
	{
		setVector(o);
	}
	template<typename U>
	fixed_vector<T, N>& operator=(std::initializer_list<U> list)
	{
		setList(std::move(list));
		return *this;
	}
	template<size_t M>
	fixed_vector<T, N>& operator=(const fixed_vector<T, M>& o)
	{
		setVector(o);
		return *this;
	}
	bool operator==(const fixed_vector<T, N>& o) const
	{
		return size() == o.size() && std::equal(cbegin(), cend(), o.cbegin());
	}
	bool operator!=(const fixed_vector<T, N>& o) const
	{
		return size() != o.size() || !std::equal(cbegin(), cend(), o.cbegin());
	}
	T& operator[](size_t i) {
		return _a[i];
	}
	const T& operator[](size_t i) const {
		return _a[i];
	}
	size_t size() const {
		return _count;
	}
	size_t count() const {
		return _count;
	}
	constexpr size_t capacity() const {
		return N;
	}
	bool empty() const {
		return _count == 0;
	}
	void push_back(const T& t) {
		if(_count < N) {
			_a[_count++] = t;
		}
	}
	void pop_back() {
		if (_count > 0) {
			_count--;
		}
	}
	void clear() {
		_count = 0;
	}
	iterator begin() {
		return _a.begin();
	}
	iterator end() {
		return _a.begin() + _count;
	}
	const_iterator begin() const {
		return cbegin();
	}
	const_iterator end() const {
		return cend();
	}
	const_iterator cbegin() const {
		return _a.cbegin();
	}
	const_iterator cend() const {
		return _a.cbegin() + _count;
	}
	reference front() {
		return _a.front();
	}
	const_reference front() const {
		return _a.front();
	}
	reference back() {
		return _count > 0 ? _a[_count-1] : _a[0];
	}
	const_reference back() const {
		return _count > 0 ? _a[_count-1] : _a[0];
	}
private:
	template<typename U>
	void setList(std::initializer_list<U>&& list) {
		_count = Q_min(list.size(), N);
		size_t i = 0;
		for (auto it = list.begin(); it != list.end(); ++it)
		{
			_a[i] = *it;
			++i;
			if (i >= _count)
				break;
		}
	}
	template<size_t M>
	void setVector(const fixed_vector<T, M>& o) {
		static_assert(M <= N, "Can't copy vector of bigger size into the vector of smaller size");

		clear();
		for (const auto& val : o) {
			push_back(val);
		}
	}
	array_type _a;
	size_t _count = 0;
};

#endif
