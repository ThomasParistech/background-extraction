/*********************************************************************************************************************
 * File : main.cpp                                                                                                   *
 *                                                                                                                   *
 * 2020 Thomas Rouch                                                                                                 *
 *********************************************************************************************************************/

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <background_extractor.h>

namespace boost_po = boost::program_options;
namespace bfs = boost::filesystem;

using Params = BackgroundExtractor::ProcessingParams;
using DisplayCb = std::function<void()>;

struct Config
{
    std::string images_dir_path_;
    std::string images_extension;
    std::string output_dir_path_;

    float resize_factor_;
};

/// @brief Utility function to parse command line attributes
bool parse_command_line(int argc, char *argv[], Config &config)
{
    const std::string short_program_desc(
        "Remove moving objects and extract background from a set of images taken\n"
        "under the same conditions (camera pose, focus, exposition time ...)\n");

    const std::string long_program_desc(
        short_program_desc +
        "\nThe app will pause on the first image coming from the input directory.\n"
        "Adjusting the parameters makes the algorithm decide which part of the \n"
        "image should be removed.\n"
        "Once the red areas correspond to the areas to remove, press any key.\n"
        "Then, the final image appears on the side, made of the pixels coming from\n"
        "the green areas.\n"
        "Repeat this step until the final image is entirely filled.\n");

    boost_po::options_description options_desc;
    boost_po::options_description base_options("Base options");
    // clang-format off
    base_options.add_options()
        ("help,h", "Produce help message.")
        ("images_dir,i", boost_po::value<std::string>(&config.images_dir_path_), "Path to the directory containing the images.")
        ("images-extension,e", boost_po::value<std::string>(&config.images_extension), "Extension of the images ('png', 'JPG' ...).")
        ("resize,r", boost_po::value<float>(&config.resize_factor_)->default_value(1.0), "Resize factor used internally to work on smaller images.")
        ;
    // clang-format on

    boost_po::options_description output_options("Output options");
    // clang-format off
    output_options.add_options()
        ("out-dir,o", boost_po::value<std::string>(&config.output_dir_path_)->default_value("/tmp/background_extraction"),
                                                              "Path of the output directory used to save the final image.")
        ;
    // clang-format on

    options_desc.add(base_options).add(output_options);

    boost_po::variables_map vm;
    try
    {
        boost_po::store(boost_po::command_line_parser(argc, argv).options(options_desc).run(), vm);
        boost_po::notify(vm);
    }
    catch (boost_po::error &e)
    {
        std::cerr << short_program_desc << std::endl;
        std::cerr << options_desc << std::endl;
        std::cerr << "Parsing error:" << e.what() << std::endl;
        return false;
    }

    if (vm.count("help"))
    {
        std::cout << short_program_desc << std::endl;
        std::cout << options_desc << std::endl;
        return false;
    }

    if (!bfs::exists(config.output_dir_path_))
    {
        try
        {
            bfs::create_directories(config.output_dir_path_);
        }
        catch (bfs::filesystem_error &e)
        {
            std::cerr << "Unable to create folder" << config.output_dir_path_ << std::endl;
            return false;
        }
    }
    if (!bfs::exists(config.images_dir_path_))
    {
        std::cerr << "The input image directory path doesn't exist: " << config.images_dir_path_ << std::endl;
        return false;
    }

    std::cout << long_program_desc << std::endl;
    return true;
}

const int blur_radius_max = 30;
const int ths_max = 40;
const int open_radius_max = 30;
const int num_final_dilations_max = 30;

Params params(11, 10, 0, 0);

int main(int argc, char **argv)
{
    Config config;
    if (!parse_command_line(argc, argv, config))
        return 1;

    BackgroundExtractor extractor(config.resize_factor_);

    std::cout << "Loading images ..." << std::endl;
    extractor.load_images(config.images_dir_path_, config.images_extension);
    std::cout << "Done." << std::endl;

    /////////////////

    const std::string main_window_name = "Background Extraction";
    const std::string result_window_name = "Result";
    cv::namedWindow(main_window_name);

    DisplayCb display_mask_cb = [&]() {
        extractor.update_mask(params);
        static cv::Mat disp_img;
        extractor.get_overlayed_reference_img(disp_img);
        cv::imshow(main_window_name, disp_img);
    };

    auto param_cb = [](int, void *display_cb_ptr) {
        (*((DisplayCb *)display_cb_ptr))();
    };

    cv::createTrackbar("Smoothing", main_window_name, &params.blur_radius, blur_radius_max, param_cb, &display_mask_cb);
    cv::createTrackbar("Thresholding", main_window_name, &params.ths, ths_max, param_cb, &display_mask_cb);
    cv::createTrackbar("Opening", main_window_name, &params.open_radius, open_radius_max, param_cb, &display_mask_cb);
    cv::createTrackbar("Dilating at the end", main_window_name, &params.num_final_dilations, num_final_dilations_max, param_cb, &display_mask_cb);

    cv::Mat disp_final_img;
    while (true)
    {
        display_mask_cb();
        cv::waitKey(0);
        const auto status = extractor.finalize_mask();
        const auto &final_img = extractor.get_final_image();
        cv::resize(final_img, disp_final_img, cv::Size(800, (800 * final_img.rows) / final_img.cols));
        cv::imshow(result_window_name, disp_final_img);
        cv::waitKey(1);

        if (status == BackgroundExtractor::Status::Success)
        {
            std::cout << " *** Success *** " << std::endl;
            std::cout << "Enough images have been provided to recover the background." << std::endl;
            const std::string write_path = (bfs::path(config.output_dir_path_) / "extracted_background.png").string();
            cv::imwrite(write_path, extractor.get_final_image());
            std::cout << "Image has been written to " << write_path << std::endl;
            break;
        }
        else if (status == BackgroundExtractor::Status::Fail)
        {
            std::cout << " *** Fail *** " << std::endl;
            std::cout << "Not enough images have been provided to recover the background." << std::endl;
            const std::string write_path = (bfs::path(config.output_dir_path_) / "failed_extracted_background.png").string();
            cv::imwrite(write_path, extractor.get_final_image());
            std::cout << "Partial image has been written to " << write_path << std::endl;
        }
    }

    return 0;
}
