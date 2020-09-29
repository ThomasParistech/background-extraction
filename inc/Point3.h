#pragma once

template<typename T>
class Point3
{
public:
	T x, y, z;

	Point3(int all = 0)
		: x(all), y(all), z(all)
	{}

	Point3(T x, T y, T z)
		: x(x), y(y), z(z)
	{}

	template<typename O>
	Point3(const Point3<O>& other)
		: x(T(other.x)), y(T(other.y)), z(T(other.z))
	{}

	T squaredNorm() {
		return x * x + y * y + z * z;
	}

	template<typename O>
	void operator+=(const Point3<O>& other) {
		x += other.x;
		y += other.y;
		z += other.z;
	}
	template<typename O>
	void operator-=(const Point3<O>& other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;
	}
	void operator/=(double k) {
		x = static_cast<T>(x / k);
		y = static_cast<T>(y / k);
		z = static_cast<T>(z / k);
	}
	template<typename O>
	void operator*=(O k) {
		x = x * k;
		y = y * k;
		z = z * k;
	}

	friend Point3<T> operator-(Point3<T> a, const Point3<T>& b) {
		a -= b;
		return a;
	}

	friend Point3<T> operator+(Point3<T> a, const Point3<T>& b) {
		a += b;
		return a;
	}
	friend Point3<T> operator * (int k, Point3<T> a) {
		a *= k;
		return a;
	}
};