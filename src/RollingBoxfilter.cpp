#include "stdafx.h"
#include "RollingBoxfilter.h"

// do the vertical reflection
int RollingBoxfilter::rowIdx(int i) {
	if(i<0)
		return -i;
	if(i>height)
		return heightMirror - i;
	return i;
}
// do the horizontal reflection
int RollingBoxfilter::colIdx(int j) {
	if (j<0)
		return -j;
	if (j>width)
		return widthMirror - j;
	return j;
}

template<typename VecIn, typename In, typename Out>
std::vector<Out> RollingBoxfilter::sumHoriz(
	VecIn & src,
	std::function<Out(In)> ptr,
	int radBox)
{
	int doubleRadBox = 2 * radBox;

	int offSet = radBox * width;
	std::vector<Out> dst;
	dst.resize(width * height + 2 * offSet);

	//iterate over the rows
	for (int i = 0; i < height; i++) {
		// initialize the first one centered at the left edge of the image: j=0
		int kIm = i * width;
		int k = kIm + offSet; //shift because there are radBox empty rows at the top at the moment
		dst[k] = ptr(src[kIm]);
		for (int j = kIm+1; j < kIm+radBox+1; j++)
			dst[k] += 2* ptr(src[j]);

		// roll the box
		for (int j = 1; j < width; j++) {
			int j1 = colIdx(j + radBox);
			int j2 = colIdx(j - radBox - 1);
			dst[k + j] = dst[k + j - 1] + ptr(src[j1]) - ptr(src[j2]);
		}
	}


	// apply the mirror on the upper rows
	// y = 2r - x
	/*
	| 0        -------
	|                |
	| r-1     --     |
	/////////  |     |
	| r+1    <--     |
	|                |
	| 2r      <-------
	*/
	for (int i1 = 0; i1 < radBox; i1++) {
		int k1 = width * i1;
		int i2 = doubleRadBox - i1;
		int k2 = width * i2;
		for (int j = 0; j < width; j++)
			dst[k1 + j] = dst[k2 + j];
	}
	
	//apply the mirror on the lower rows
	// y = 2h+2r-2 - x
	/*
	| h-1       <-------
	|                   |
	| h+r-2     <--     |
	/////////     |     |
	| h+r        --     |
	|                   |
	| h+2r-1     -------
	*/
	
	for (int i1 = height + radBox; i1 < height + doubleRadBox; i1++) {
		int k1 = width * i1;
		int i2 = heightMirror + doubleRadBox - i1;
		int k2 = width * i2;
		for (int j = 0; j < width; j++)
			dst[k1 + j] = dst[k2 + j];
	}
	return dst;
}

template<typename VecIn, typename In, typename Out>
std::vector<Out> RollingBoxfilter::applyKernel(
	VecIn & src,
	std::function<Out(In)> ptr,
	int radBox) 
{
	int doubleRadBox = 2*radBox;
	std::vector<Out> R = sumHoriz(src, ptr, radBox);
	std::vector<Out> dst;
	dst.resize(width*height);

	int offSet = radBox * width;

	//iterate over the columns
	for (int j = 0; j < width; j++) {
		// initialize the first one centered at the top edge of the image: i=0
		dst[j] = R[j + offSet];
		for (int i = 1, k = width; i < radBox + 1; i++, k += width)
			dst[j] += 2 * R[j + k + offSet];
		
		// roll the box
		for (int i = 1, k = width; i<height; i++, k += width) {
			int i1 = rowIdx(i + radBox);
			int i2 = rowIdx(i - radBox - 1);
			dst[j + k] = dst[j + k - width] + R[j + offSet + i1 * width] - R[j + offSet + i2 * width];
		}
	}
	return dst;
}

template <typename In>
Mat3i RollingBoxfilter::meanBlur(const Point3<In> * src, int radBox) {
	std::function<Point3i(Point3<In>)> meanFn = [](Point3<In> pt) { return Point3i(pt); };
	return applyKernel(src, meanFn, radBox);
};

template <typename In>
Mat3i RollingBoxfilter::meanBlur(std::vector<Point3<In>> & src, int radBox) {
	std::function<Point3i(Point3<In>)> meanFn = [](Point3<In> pt) { return Point3i(pt); };
	return applyKernel(src, meanFn, radBox);
};


template <typename In>
Mat3b RollingBoxfilter::meanBlurNorm(const Point3<In> * src, int radBox) {
	Mat3i C = meanBlur(src, radBox);
	
	double factDiv = 1/(2 * radBox + 1);
	factDiv *= factDiv;

	Mat3b res;
	res.resize(C.size());
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int k = i * width + j;
			res[k] = C[k];
			res[k] *= factDiv;
		}
	}
	return res;
};

Matb RollingBoxfilter::morph(Matb& src, valFn ptr, int radBox) {
	Mati C = applyKernel(src, ptr, radBox);

	Matb res;
	res.resize(height*width);
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int k = i * width + j;
			res[k] = (C[k] == 0 ? 255 : 0);
		}
	}
	return res;
}
Matb RollingBoxfilter::dilation(Matb& src, int radBox) {
	std::function<uint8_t(uint8_t)> dilationFn = [](uint8_t val) { return (val == 255 ? 0 : 1); };
	return morph(src, dilationFn, radBox);
}
Matb RollingBoxfilter::erosion(Matb& src, int radBox) {
	std::function<uint8_t(uint8_t)> erosionFn = [](uint8_t val) { return (val == 255 ? 1 : 0); };
	return morph(src, erosionFn, radBox);
}