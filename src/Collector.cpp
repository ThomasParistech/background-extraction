#include "stdafx.h"
#include "Collector.h"

Collector::~Collector()
{
}

// load one image at a time
void Collector::addImg(const uint8_t* BGR, const uint8_t* BGR_small) {
	imgs.push_back(convertToPoint3(BGR_small));
	originalImgs.push_back(convertToPoint3(BGR));
	n++;
};


// when all the images have been loaded
// Create an ObjectRemoval with the Image Reference at the index idx
// and display the RedGreen Image with the default params
uint8_t*  Collector::beginImg(int idx, int threshold, int radMorph, int radBlur) {
	currentIdx = idx;
	objRemove = ObjectRemoval(imgs[idx], width, height, n);
	for (int i = 0; i < n; i++) {
		if (i != idx)
			objRemove.addImg(imgs[i]);
	}
	return convertToByte(objRemove.firstUpdate(threshold, radMorph, radBlur));
}
// update with new thresh or radisu for Erosion
uint8_t*  Collector::updateImg(int threshold, int radMorph) {
	return convertToByte(objRemove.update(threshold, radMorph));
}
// update with new radius for Blurring
uint8_t*  Collector::updateImg(int threshold, int radMorph, int radBlur) {
	return convertToByte(objRemove.update(threshold, radMorph, radBlur));
}

// save the mask at the end, when we're happy with the RedGreen image
void Collector::saveMask() {
	masks[currentIdx] = objRemove.getMask();
}

// the masks are defined on reduced images
// they need to be stretched to the original size of the images
void resizeTheMasks() {
	//upsampling


	//to do


}

// when all the masks have been computed
// merge all images according to the masks
uint8_t* Collector::merge() {
	Mat3b res;
	res.resize(width*height);
	for (int i = 0, i0 = 0; i < height; ++i, i0 += width) {
		for (int j = 0; j < width; ++j) {
			int j0 = i0 + j;
			int nbr = 0;
			for (int k = 0; k < n; k++) {
				if (masks[k][j0] == 255) {
					nbr++;
					res[j0] += imgs[k][j0];
				}
			}
			if (nbr != 0)
				res[j0] /= nbr;
			else
				res[j0].z = 255; //red point
		}
	}
	return convertToByte(res);
}


template<typename T>
T* Collector::convertToByte(std::vector<Point3<T>> vec) {
	return reinterpret_cast<T*>(vec.data());
}

template<typename T>
const Point3<T>* Collector::convertToPoint3(const T * vec) {
	return reinterpret_cast<const Point3<T>* >(vec);
}