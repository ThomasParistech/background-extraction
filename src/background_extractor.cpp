/*********************************************************************************************************************
 * File : background_extractor.cpp                                                                                   *
 *                                                                                                                   *
 * 2020 Thomas Rouch                                                                                                 *
 *********************************************************************************************************************/

#include <assert.h>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <boost/filesystem.hpp>

#include "background_extractor.h"

namespace bfs = boost::filesystem;

BackgroundExtractor::ProcessingParams::ProcessingParams(int blur_radius,
                                                        int ths,
                                                        int open_radius,
                                                        int num_final_dilations) : blur_radius(blur_radius),
                                                                                   ths(ths),
                                                                                   open_radius(open_radius),
                                                                                   num_final_dilations(num_final_dilations) {}

void BackgroundExtractor::ProcessingParams::reset()
{
    blur_radius = -1;
    ths = -1;
    open_radius = -1;
    num_final_dilations = -1;
}

BackgroundExtractor::BackgroundExtractor(float resize_factor,
                                         cv::Vec3b bg_color) : resize_factor_(resize_factor),
                                                               bg_color_(bg_color),
                                                               last_params_(-1, -1, -1, -1),
                                                               crt_id_ref_(0)

{
}

bool BackgroundExtractor::load_images(const std::string &dir_path, const std::string &image_extension)
{
    reset();

    // Find image filenames in the directory
    std::vector<cv::String> filenames;
    const std::string search_path = (bfs::path(dir_path) / ("*." + image_extension)).string();
    cv::glob(search_path, filenames);
    const int n = filenames.size();
    if (n == 0)
    {
        std::cerr << "There's no images with a " << image_extension << " extension in the folder " << dir_path << std::endl;
        return false;
    }

    // Load images
    original_imgs_.reserve(n);
    for (size_t i = 0; i < n; i++)
        original_imgs_.emplace_back(cv::imread(filenames[i]));

    original_size_ = original_imgs_[0].size();

    // Make sure they all have the same size
    for (const auto &img : original_imgs_)
    {
        if (img.size() != original_size_)
        {
            std::cerr << "Images must all have the same size." << std::endl;
            return false;
        }
    }

    // Resize images
    height_ = int(resize_factor_ * original_size_.height);
    width_ = int(resize_factor_ * original_size_.width);
    resized_imgs_.reserve(n);
    for (size_t i = 0; i < n; i++)
    {
        cv::Mat resized_img;
        cv::resize(original_imgs_[i], resized_img, cv::Size(width_, height_), cv::INTER_AREA);
        resized_imgs_.emplace_back(std::move(resized_img));
    }

    // Pre allocate images containing the differences
    uchar_blurred_diffs_.reserve(n - 1);
    for (size_t i = 0; i < n - 1; i++)
        uchar_blurred_diffs_.emplace_back(width_, height_);

    // Set the first reference image
    crt_id_ref_ = 0;
    last_params_.reset();

    // Init final image
    final_img_.create(original_size_.height, original_size_.width, CV_8UC3);
    final_img_.setTo(bg_color_);

    no_info_mask_.create(height_, width_);
    no_info_mask_.setTo(cv::Scalar(255));
    return true;
}

void BackgroundExtractor::update_mask(const ProcessingParams &params)
{
    assert(!uchar_blurred_diffs_.empty());

    bool has_changed = false;
    // 1) Blur (Update uchar_blurred_diffs_)
    if (params.blur_radius != last_params_.blur_radius)
    {
        // Apply blur on the reference image
        const cv::Size kernel_blur(2 * params.blur_radius + 1, 2 * params.blur_radius + 1);
        cv::blur(resized_imgs_[crt_id_ref_], tmp_blurred_ref_, kernel_blur);

        // Compute the intensity of the blurred difference against all other images
        for (size_t i = 0; i < resized_imgs_.size(); i++)
        {
            if (i != crt_id_ref_)
            {
                // Keep uchar images
                cv::blur(resized_imgs_[i], tmp_cv8uc3_, kernel_blur);
                cv::absdiff(tmp_blurred_ref_, tmp_cv8uc3_, tmp_cv8uc3_);
                auto &dst = uchar_blurred_diffs_[img_to_diff_id(i)];
                cv::cvtColor(tmp_cv8uc3_, dst, cv::COLOR_BGR2GRAY);
            }
        }
        has_changed = true;
    }

    // 2) Threshold (update mask_before_morph_)
    if (has_changed || params.ths != last_params_.ths)
    {

        // Reset mask
        mask_before_morph_.create(height_, width_);
        mask_before_morph_.setTo(0);
        tmp_mask_.create(height_, width_);

        for (int k = 0; k < resized_imgs_.size() - 1; k++)
        {
            // Compute the mask by applying a threshold on the grayscale blurred difference
            tmp_mask_.setTo(0);
            cv::threshold(uchar_blurred_diffs_[k], tmp_mask_, params.ths, 255, cv::THRESH_BINARY_INV); // 255 if below ths

            // If an area in a mask_k is white, then it should also be white in the final mask
            cv::bitwise_or(mask_before_morph_, tmp_mask_, mask_before_morph_);
        }
        has_changed = true;
    }

    // 3) Open and Dilate (update mask_)
    if (has_changed || params.ths != last_params_.ths || params.open_radius != last_params_.open_radius || params.num_final_dilations != last_params_.num_final_dilations)
    {
        mask_before_morph_.copyTo(mask_);

        // Opening (to remove small areas)
        const cv::Mat opening_kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                                                 cv::Size(2 * params.open_radius + 1, 2 * params.open_radius + 1),
                                                                 cv::Point(params.open_radius, params.open_radius));
        if (params.open_radius > 0)
        {
            cv::erode(mask_, mask_, opening_kernel);
            cv::dilate(mask_, mask_, opening_kernel);
        }

        // Erosion (make areas grow)
        const cv::Mat erosion_kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                                                 cv::Size(5, 5),
                                                                 cv::Point(1, 1));
        for (size_t i = 0; i < params.num_final_dilations; i++)
            cv::erode(mask_, mask_, erosion_kernel);

        valid_mask_ = true;
    }

    // Keep in memory last processing parameters
    last_params_ = params;
}

BackgroundExtractor::Status BackgroundExtractor::finalize_mask()
{
    assert(valid_mask_);
    valid_mask_ = false;

    // Overwrite final image using the mask (only areas that haven't been updated yet)
    cv::bitwise_and(mask_, no_info_mask_, tmp_mask_);
    cv::resize(tmp_mask_, original_size_mask_, original_size_, cv::INTER_CUBIC);
    original_imgs_[crt_id_ref_].copyTo(final_img_, original_size_mask_);

    // Update no info mask
    no_info_mask_.setTo(0, mask_);

    // Check if all the pixels have been recovered
    if (cv::countNonZero(no_info_mask_) < 1)
        return Status::Success;

    // Set the next reference image
    crt_id_ref_++;
    last_params_.reset();

    if (crt_id_ref_ < resized_imgs_.size())
        return Status::Continue;
    else
        return Status::Fail;
}

const cv::Mat &BackgroundExtractor::get_final_image()
{

    return final_img_;
}

void BackgroundExtractor::get_overlayed_reference_img(cv::Mat &img)
{
    static cv::Mat color_mask;
    img.create(height_, width_, CV_8UC3);
    color_mask.create(height_, width_, CV_8UC3);
    color_mask.setTo(cv::Vec3b(0, 0, 255));
    assert(valid_mask_);
    color_mask.setTo(cv::Vec3b(0, 255, 0), mask_);
    cv::addWeighted(resized_imgs_[crt_id_ref_], 0.7, color_mask, 0.3, 0, img);

    color_mask.setTo(cv::Scalar::all(0));
    color_mask.setTo(cv::Scalar::all(255), no_info_mask_);

    cv::addWeighted(img, 0.5, color_mask, 0.5, 0, img);
}

void BackgroundExtractor::reset()
{
    original_imgs_.clear();
    resized_imgs_.clear();
    uchar_blurred_diffs_.clear();

    crt_id_ref_ = 0;
    last_params_.reset();
    valid_mask_ = false;
}

int BackgroundExtractor::img_to_diff_id(int img_id)
{
    return (img_id < crt_id_ref_ ? img_id : img_id - 1);
}
