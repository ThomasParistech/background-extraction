#pragma once
#include "ObjectRemoval.h"

class Collector
{
public:
	Collector() {};
	Collector(int originalWidth, int width, int originalHeight, int height) : 
		originalWidth{ originalWidth },
		width{ width },
		originalHeight{ originalHeight },
		height{ height }
	{ n = 0; };
	~Collector();

	void addImg(const uint8_t* BGR, const uint8_t* BGR_small);

	uint8_t* beginImg(int idx, int threshold, int radMorph, int radBlur);
	uint8_t* updateImg(int threshold, int radMorph, int radBlur);
	uint8_t* updateImg(int threshold, int radMorph);

	void saveMask();

	uint8_t* merge();

	template<typename T>
	static T* convertToByte(std::vector<Point3<T>> vec);

	template<typename T>
	static const Point3<T>* convertToPoint3(const T * vec);

	ObjectRemoval objRemove;
	Arrs3b imgs;
	Arrs3b originalImgs;
	Matsb masks;
	int width, height;
	int originalWidth, originalHeight;
	int n;
	int currentIdx;
};

