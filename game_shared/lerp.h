#pragma once
#ifndef LERP_H
#define LERP_H

#include <cmath>

template<typename _Float>
typename std::enable_if<std::is_floating_point<_Float>::value, _Float>::type
lerp(_Float __a, _Float __b, _Float __t)
{
	if (std::isnan(__a) || std::isnan(__b) || std::isnan(__t))
		return std::numeric_limits<_Float>::quiet_NaN();
	else if ((__a <= _Float{0} && __b >= _Float{0})
			 || (__a >= _Float{0} && __b <= _Float{0}))
	// ab <= 0 but product could overflow.
		return std::fma(__t, __b, (_Float{1} - __t) * __a);
	else if (__t == _Float{1})
		return __b;
	else
	{ // monotonic near t == 1.
		const auto __x = std::fma(__t, __b - __a, __a);
		return (__t > _Float{1}) == (__b > __a)
				? std::max(__b, __x)
				: std::min(__b, __x);
	}
}

#endif
