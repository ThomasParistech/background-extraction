#pragma once
#include <vector>
#include "RollingBoxfilter.h"
#include "Point3.h"
#include <algorithm>

class ObjectRemoval
{

public:
	ObjectRemoval() {};
	ObjectRemoval(Arr3b BGR_Ref, int _width, int _height, int nbrImg);
	~ObjectRemoval();

	void addImg(Arr3b BGR_Img);
	Mat3b firstUpdate(int threshold, int radMorph, int radBlur);
	Mat3b update(int threshold, int radMorph, int radBlur);
	Mat3b update(int threshold, int radMorph);
	void logicalOR(Matb & a, Matb &b);
	Mat3i matDiff(Arr3b & a, Arr3b &b);
	Mati diffC3toC1(Mat3i &a);

	Matb getMask() { return mask; }
private:
	Arr3b imgRef;
	Mats3i diffs;
	Matsi blurredDiffs;
	Matb mask;
	int width, height;
	int sizeVec;
	int n; //nbr images

	RollingBoxfilter box;
};

