/*********************************************************************************************************************
 * File : background_extractor.h                                                                                     *
 *                                                                                                                   *
 * 2020 Thomas Rouch                                                                                                 *
 *********************************************************************************************************************/

#ifndef BACKGROUND_EXTRACTOR_H
#define BACKGROUND_EXTRACTOR_H

#include <opencv2/core/mat.hpp>

class BackgroundExtractor
{
public:
    enum Status
    {
        Continue, ///< Keep going
        Success,  ///< Final image is ready
        Fail      ///< There's no images left and there remains uncovered parts in the final image
    };

    struct ProcessingParams
    {
        ProcessingParams(int blur_radius, int ths, int open_radius, int num_final_erosions);

        void reset();

        int blur_radius;        ///< Blurring kernel size. The larger the radius, the less noisy the mask is
        int ths;                ///< Grayscale threshold in [0,255] defining if colors are similar. A very low
                                /// threshold will define everything as foreground, while a very large threshold will
                                /// define everything as background
        int open_radius;        ///< Kernel size of the morphological opening (Erosion/Dilation). Parts of the mask
                                /// that are smaller than the kernel size are removed
        int num_final_erosions; ///< Number of consecutive morphological 5x5 erosions to apply at the end.
                                /// Once small areas have been removed, it will let areas grow to make sure the selection
                                /// isn't too tight
    };

    /// @brief Constructor
    /// @param resize_factor Work with downsampled images to determine selection masks
    /// @note The final image will still have the same size as the input image
    /// @param bg_color Background color used to show pixels that haven't been recovered yet in the final image
    /// @note The color should have a high contrast with respect to the input image to help it stand out
    BackgroundExtractor(float resize_factor = 1.f, cv::Vec3b bg_color = cv::Vec3b(0, 0, 255));

    ~BackgroundExtractor() = default;

    /// @brief Loads images from a directory and resizes them for the next processing steps
    /// @param dir_path Path to the directory containing the images to load
    /// @param image_extension Extension of the image file ("JPG", "jpeg", "png", "bmp", ...)
    /// @return true if the images have been correctly loaded
    bool load_images(const std::string &dir_path, const std::string &image_extension);

    /// @brief Estimates the background mask in the reference image
    ///
    /// 1) Blur the difference images (reference image against all the other images)
    /// 2) Apply a threshold on the grayscale intensity of the blurred differences
    /// 3) Perform a morphological opening to remove small noisy areas
    ///
    /// Once we get a binary mask for each image (0: different / 255: identical),
    /// we concatenate them all using a logical OR.
    ///
    /// @note If the parameters haven't changed since the last call to this function, the processing is
    /// skipped and the method returns the previous version of the mask
    /// @param params Processing parameters
    void update_mask(const ProcessingParams &params);

    /// @brief Gets the current reference image overlayed with selection_masks
    /// @param img Output image
    void get_overlayed_reference_img(cv::Mat &img);

    /// @brief Saves current mask and selects the next reference image
    /// @return Process status
    Status finalize_mask();

    /// @brief Gets the final image of the background
    const cv::Mat &get_final_image();

private:
    /// @brief Clears vectors of images and resets reference ID to 0
    void reset();

    int img_to_diff_id(int img_id);

    std::vector<cv::Mat> original_imgs_;
    std::vector<cv::Mat> resized_imgs_;

    cv::Mat tmp_blurred_ref_, tmp_cv8uc3_;
    std::vector<cv::Mat_<uint8_t>> uchar_blurred_diffs_;

    cv::Mat_<uint8_t> tmp_mask_;
    cv::Mat_<uint8_t> mask_before_morph_;

    cv::Mat_<uint8_t> mask_; ///< Mask
    cv::Mat_<uint8_t> original_size_mask_;

    cv::Mat final_img_;
    cv::Mat_<uint8_t> no_info_mask_; ///< Parts of the final image that haven't been updated yet

    int crt_id_ref_;
    bool valid_mask_;

    int last_blur_radius_;
    int last_ths_;
    int last_open_radius_;

    cv::Size original_size_;
    int width_, height_;
    const float resize_factor_;

    cv::Vec3b bg_color_;

    ProcessingParams last_params_;
};

#endif // BACKGROUND_EXTRACTOR_H
