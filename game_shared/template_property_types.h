#pragma once
#ifndef TEMPLATE_VALUE_TYPES_H
#define TEMPLATE_VALUE_TYPES_H

struct Color3
{
	constexpr Color3(): r(0), g(0), b(0) {}
	constexpr Color3(int red, int green, int blue): r(red), g(green), b(blue) {}
	int r;
	int g;
	int b;

	constexpr inline bool operator==(Color3 o) const
	{
		return IsEqual(o);
	}
	constexpr inline bool operator!=(Color3 o) const
	{
		return !IsEqual(o);
	}
private:
	constexpr inline bool IsEqual(const Color3& o) const {
		return r == o.r && g == o.g && b == o.b;
	}
};

template <typename N>
struct NumberRange
{
	constexpr NumberRange(): min(), max() {}
	constexpr NumberRange(N mini, N maxi): min(mini), max(maxi) {}
	constexpr NumberRange(N val): min(val), max(val) {}
	N min;
	N max;

	constexpr inline bool operator==(const NumberRange<N>& o) const {
		return IsEqual(o);
	}
	constexpr inline bool operator==(const N& o) const {
		return IsEqual(o);
	}
	constexpr inline bool operator!=(const NumberRange<N>& o) const {
		return !IsEqual(o);
	}
	constexpr inline bool operator!=(const N& o) const {
		return !IsEqual(o);
	}
	inline NumberRange<N>& operator*=(const N f) {
		min *= f;
		max *= f;
		return *this;
	}
	constexpr inline bool IsProperRange() const {
		return max > min;
	}
	constexpr inline bool IsPositive() const {
		return min >= 0 && max > 0;
	}
	constexpr inline N Middle() const {
		return IsProperRange() ? (max - min) / 2 : min;
	}
private:
	constexpr inline bool IsEqual(const NumberRange<N>& o) const {
		return min == o.min && max == o.max;
	}
	constexpr inline bool IsEqual(const N& o) const {
		return min == o && max == o;
	}
};

template<typename N, typename M>
constexpr NumberRange<decltype(N() * M())> operator*(const NumberRange<N>& r, M f) {
	return {r.min * f, r.max * f};
}

template<typename N>
NumberRange<N> RangeSum(const NumberRange<N>& a, const NumberRange<N>& b)
{
	NumberRange<N> result = a;
	if (b.IsProperRange())
	{
		if (!result.IsProperRange())
		{
			result.max = result.min;
		}
		result.min += b.min;
		result.max += b.max;
	}
	else
	{
		if (result.IsProperRange())
		{
			result.min += b.min;
			result.max += b.min;
		}
		else
		{
			result.min += b.min;
		}
	}
	return result;
}

typedef NumberRange<float> FloatRange;
typedef NumberRange<int> IntRange;

struct PlayerShake
{
	PlayerShake():
		radius(192),
		duration(0.0f),
		frequency(160.0f),
		amplitude(6) {}
	int radius;
	float duration;
	float frequency;
	int amplitude;
	inline bool IsDefined() const {
		return duration > 0;
	}
};

#endif
