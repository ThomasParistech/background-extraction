#pragma once
#include <vector>
#include <functional>
#include "Point3.h"

typedef std::vector<uint8_t> Matb; // byte
typedef std::vector<int32_t> Mati; // int
typedef std::vector<Matb> Matsb;
typedef std::vector<Mati> Matsi;

typedef Point3<uint8_t> Point3b;
typedef Point3<int32_t> Point3i;

typedef const Point3b* Arr3b;
typedef std::vector<Arr3b> Arrs3b;

typedef std::vector<Point3b> Mat3b; // byte
typedef std::vector<Point3i> Mat3i; // int
typedef std::vector<Mat3b> Mats3b;
typedef std::vector<Mat3i> Mats3i;

typedef std::function<int(uint8_t)> valFn;

class RollingBoxfilter
{
public:
	RollingBoxfilter() {};
	RollingBoxfilter(int width, int height) :
		width{ width }, 
		height{ height } 
	{
		heightMirror = 2 * (height - 1);
		widthMirror = 2 * (width - 1);
	};

	~RollingBoxfilter() {} ;

	// Handling image boundaries
	// gfedcb|abcdefgh|gfedcba
	int rowIdx(int i);
	int colIdx(int j);

	// VecIn can be : std::vector<In> or const In*
	template<typename VecIn, typename In, typename Out>
	std::vector<Out> sumHoriz(
		VecIn & src,
		std::function<Out(In)> ptr,
		int radBox);

	// VecIn can be : std::vector<In> or const In*
	template<typename VecIn, typename In, typename Out>
	std::vector<Out> applyKernel(
		VecIn & src,
		std::function<Out(In)> ptr,
		int radBox);

	//Arr3b or Arr3i as argument
	template <typename In>
	Mat3i meanBlur(const Point3<In> * src, int radBox);
	//Mat3b or Mat3i as argument
	template <typename In>
	Mat3i meanBlur(std::vector<Point3<In>> & src, int radBox);
	template <typename In>
	Mat3b meanBlurNorm(const Point3<In> * src, int radBox); //divide by the nbr of cells in the box

	Matb morph(Matb & src, valFn ptr, int radBox);
	Matb erosion(Matb& src, int radBox);
	Matb dilation(Matb& src, int radBox);

private:
	int height, width;
	int heightMirror, widthMirror;
};

