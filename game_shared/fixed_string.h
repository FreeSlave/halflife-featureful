#pragma once
#ifndef FIXED_STRING_H
#define FIXED_STRING_H

#include <cstddef>
#include <type_traits>
#include "string_utils.h"

template<size_t N>
struct fixed_string
{
	typedef char value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef char& reference;
	typedef const char& const_reference;
	typedef char* pointer;
	typedef const char* const_pointer;
	typedef const char* iterator;
	typedef const char* const_iterator;

	fixed_string() {
		init();
	}
	fixed_string(const char* str) {
		setString(str);
	}
	template<size_t K>
	fixed_string(const fixed_string<K>& o) {
		static_assert(K <= N, "Can't convert string of bigger size to the string of smaller size");
		setString(o.c_str());
	}
	fixed_string<N>& operator=(const char* str) {
		setString(str);
		return *this;
	}
	template<size_t K>
	fixed_string<N>& operator=(const fixed_string<K>& o) {
		static_assert(K <= N, "Can't convert string of bigger size to the string of smaller size");
		setString(o.c_str());
		return *this;
	}
	bool operator==(const char* str) const {
		return strcmp(_a, str) == 0;
	}
	bool operator!=(const char* str) const {
		return strcmp(_a, str) != 0;
	}
	bool operator<=(const char* str) const {
		return strcmp(_a, str) <= 0;
	}
	bool operator<(const char* str) const {
		return strcmp(_a, str) < 0;
	}
	bool operator>=(const char* str) const {
		return strcmp(_a, str) >= 0;
	}
	bool operator>(const char* str) const {
		return strcmp(_a, str) > 0;
	}
	template<size_t K>
	bool operator==(const fixed_string<K>& s) const {
		return strcmp(c_str(), s.c_str()) == 0;
	}
	template<size_t K>
	bool operator!=(const fixed_string<K>& s) const {
		return strcmp(c_str(), s.c_str()) != 0;
	}
	template<size_t K>
	bool operator<=(const fixed_string<K>& s) const {
		return strcmp(c_str(), s.c_str()) <= 0;
	}
	template<size_t K>
	bool operator<(const fixed_string<K>& s) const {
		return strcmp(c_str(), s.c_str()) < 0;
	}
	template<size_t K>
	bool operator>=(const fixed_string<K>& s) const {
		return strcmp(c_str(), s.c_str()) >= 0;
	}
	template<size_t K>
	bool operator>(const fixed_string<K>& s) const {
		return strcmp(c_str(), s.c_str()) > 0;
	}
	char operator[](size_t i) const {
		return _a[i];
	}
	size_t size() const {
		return _size;
	}
	bool empty() const {
		return _size == 0;
	}
	void clear() {
		init();
	}
	iterator begin() const {
		return cbegin();
	}
	iterator end() const {
		return cend();
	}
	const_iterator cbegin() const {
		return _a;
	}
	const_iterator cend() const {
		return _a + _size;
	}
	const char* c_str() const {
		return _a;
	}
private:
	void init()
	{
		_size = 0;
		_a[0] = '\0';
	}
	void setString(const char* str)
	{
		if (str == nullptr)
		{
			init();
		}
		else
		{
			strncpyEnsureTermination(_a, str);
			_size = strlen(_a);
		}
	}
	char _a[N];
	size_t _size;
};

#endif
