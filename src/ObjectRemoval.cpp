#include "stdafx.h"
#include "ObjectRemoval.h"

// set the ref image
ObjectRemoval::ObjectRemoval(Arr3b BGR, int _width, int _height, int nbrImg) :
	imgRef {BGR},
	width{_width},
	height{_height},
	n{ nbrImg - 1}, //image reference is treated in a different way
	sizeVec{ _width * _height }
{
	diffs.reserve(n);
}

ObjectRemoval::~ObjectRemoval()
{
}

void ObjectRemoval::addImg(Arr3b BGR_Img){
	//compute the diff between the ref image and BGR_Img
	diffs.push_back(matDiff(imgRef, BGR_Img));
}

Mat3b ObjectRemoval::firstUpdate(int threshold, int radMorph, int radBlur) {
	box = RollingBoxfilter(width, height);
	blurredDiffs.reserve(n);
	for (int i = 0; i < n; i++) {
		auto bl = box.meanBlur(diffs[i], radBlur);
		blurredDiffs.push_back(diffC3toC1(bl));
	}
	return update(threshold, radMorph);
}

Mat3b ObjectRemoval::update(int threshold, int radMorph, int radBlur)
{
	for (int i = 0; i < n; i++) {
		auto bl = box.meanBlur(diffs[i], radBlur);
		blurredDiffs[i] = diffC3toC1(bl);
	}
	return update(threshold, radMorph);
}


Mat3b ObjectRemoval::update(int threshold, int radMorph)
{
	//black mask at the beginning
	mask.resize(sizeVec, 0);

	// Compute the mask
	for (int k = 0; k< blurredDiffs.size(); k++) {
		// compute the mask by applying a threshold on the blurred difference
		Matb maskTemp;
		maskTemp.resize(sizeVec, 0);
		for (int i = 0, i0 = 0; i < height; i++, i0 += width) {
			for (int j = 0; j < width; j++) {
				maskTemp[i0 + j] = (blurredDiffs[k][i0 + j] < threshold ? 255 : 0);
			}
		}
		// Morph - Opening
		maskTemp = box.erosion(maskTemp, radMorph);
		maskTemp = box.dilation(maskTemp, radMorph);

		// Store the info in mask
		// If one maskTemp contains a white area, then mask should have it too
		logicalOR(mask, maskTemp);
	}

	// Apply a filter on the reference image to show the mask
	// White areas of the mask will look green, 
	// whereas black areas will look red
	Mat3b imgRedGreen;
	imgRedGreen.resize(sizeVec);
	for (int i = 0, i0 = 0; i < height; ++i, i += width) {
		for (int j = 0; j < width; ++j) {
			int k = i0 + j;
			imgRedGreen[k] = imgRef[k];

			//assuming bgr
			if (mask[k] == 255) //green
				imgRedGreen[k].y = std::max(std::min(3 * imgRedGreen[k].y, 255), 0);
			else // red
				imgRedGreen[k].z = std::max(std::min(3 * imgRedGreen[k].z, 255), 0);
		}
	}
	return imgRedGreen;
}

// a and b are Matb with 1 channel (grey scale images)
//put the result in a
void ObjectRemoval::logicalOR(Matb & a, Matb &b) {
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int k = i * width + j;
			if (b[k] == 255)
				a[k] = 255;
		}
	}
}

Mat3i ObjectRemoval::matDiff(Arr3b & a, Arr3b &b) {
	Mat3i res;
	res.resize(height * width);
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int k = i*width + j;
			res[k] = Point3i(a[k] - b[k]);
		}
	}
	return res;
}

// res = squared sum of each channel
Mati ObjectRemoval::diffC3toC1(Mat3i & src) {
	Mati res;
	res.resize(height * width);
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			int k = i * width + j;
			res[k] = src[k].squaredNorm();
		}
	}
	return res;
}