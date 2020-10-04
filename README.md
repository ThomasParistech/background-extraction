# Background Extraction

Aim of this app is to combine multiple pictures of the same scene to get rid of moving objects passing in front of the camera and ruining our picture.

Make sure the pictures have all been taken under the same conditions (camera pose, focus, exposition time ...). Use a tripod for that.

The user can adjust parameters through the GUI to help the algorithm decide which part of the image should be removed.

## 1 - Installation

First, clone the repository.
```
git clone https://github.com/ThomasParistech/background-extraction.git
```
Then, go to the directory and compile it.
```
cd background-extraction
mkdir build
cd build
cmake ..
make -j6
```
Please note that CMakeLists.txt is configured in a such way that the executable is generated in the "bin" directory.

## 2 - Running

Go to the build directory and launch the main app. Example:
```
bin/main -i ../images/MairieAmberieu -e "JPG" -r 0.15
```
Check the help for additional information
```
bin/main -h
```

The app will pause on the first image coming from the input directory.
Adjusting the parameters makes the algorithm decide which part of the image should be removed.
Once the red areas correspond to the areas to remove, press any key.
Then, the final image appears on the side, made of the pixels coming from the green areas.
Repeat this step until the final image is entirely filled.
![](./images/overview_app.gif)

## 3 - Algorithm

We assume that each area of the background is at least visible on two images in the dataset. Otherwise there's no way to distinguish it from moving objects.

The algorithm iterates over the images dataset and compares each image against the others. If some pixels areas in this image can't be matched with other images, then they probably do not belong to the background. That way we can define a binary mask choosing which region to select in the image.

Since the notion of "similarity" might vary a lot from a dataset to another, we ask the user to adjust the settings for more robustness.

There are 4 parameters:
- Blurring: We blur the input images before comparing them. The larger the radius of the kernel, the less noisy the mask is
- Thresholding: We apply a threshold on the grayscale intensity of the blurred differences. A very low threshold will define everything as foreground, while a very large threshold will define everything as background
- Opening: Kernel size of the morphological opening (Erosion/Dilation). Parts of the mask that are smaller than the kernel size are removed
- Dilation:  Number of consecutive morphological 5x5 dilations to apply at the end. Once small areas have been removed, it will let areas grow to make sure the selection isn't too tight

![](./images/overview_processing.gif)


