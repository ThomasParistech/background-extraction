/*********************************************************************************************************************
 * File : capsules_solver_app.cpp                                                                                    *
 *                                                                                                                   *
 * 2020 Thomas Rouch                                                                                                 *
 *********************************************************************************************************************/

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <iostream>

#include <capsules_solver.h>

namespace boost_po = boost::program_options;
namespace fs = boost::filesystem;

struct Config
{
    std::string image_path_;
    std::string capsules_dir_path_;
    int n_rows_;

    bool display_errors_;
    std::string output_dir_path_;
};

/// @brief Utility function to parse command line attributes
bool parse_command_line(int argc, char *argv[], Config &config)
{
    const std::string short_program_desc(
        "Find the optimal arrangement of champagne capsules to represent a given photograph.\n");

    const std::string long_program_desc(
        short_program_desc +
        "\nFirst, the app pauses and shows you the capsules grid superimposed on the input image, \n"
        "to see which part of the image will be used for the matching. This grid is composed of\n"
        "rows of circles one above the other and will be filled with champagne capsules once the \n"
        "optimisation is done.\n"
        "Press any key to continue.\n"
        "Then, the algorithms computes the optimal combination and displays its solution.\n"
        "If the option has been enabled, the error map is then displayed.\n");

    boost_po::options_description options_desc;
    boost_po::options_description base_options("Base options");
    // clang-format off
    base_options.add_options()
        ("help,h", "Produce help message.")
        ("input-image,i", boost_po::value<std::string>(&config.image_path_), "Path to an image file.")
        ("input-capsules,c", boost_po::value<std::string>(&config.capsules_dir_path_), "Path to the directory containing the loaded capsules.")
        ("nbr-rows,r", boost_po::value<int>(&config.n_rows_), "Number of capsules rows of the final composition.")
        ;
    // clang-format on

    boost_po::options_description output_options("Output options");
    // clang-format off
    output_options.add_options()
        ("display-errors,e", boost_po::value(&config.display_errors_)->default_value(false), "Activate computation and display of the error map or not.")
        ("out-dir,o", boost_po::value<std::string>(&config.output_dir_path_)->default_value("/tmp/placomosaic"), "Path of the output directory used to save the images and generate an html "
                                                                       "grid listing the ids of the capsules used in the composition.")
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

    if (!fs::exists(config.output_dir_path_))
    {
        try
        {
            fs::create_directories(config.output_dir_path_);
        }
        catch (fs::filesystem_error &e)
        {
            std::cerr << "Unable to create folder" << config.output_dir_path_ << std::endl;
            return false;
        }
    }
    if (config.n_rows_ <= 0)
    {
        std::cerr << "The number of rows must be strictly positive. Got " << config.n_rows_ << "." << std::endl;
        return false;
    }
    if (!fs::exists(config.image_path_))
    {
        std::cerr << "The input image path doesn't exist: " << config.image_path_ << std::endl;
        return false;
    }
    if (!fs::exists(config.capsules_dir_path_))
    {
        std::cerr << "The input capsules directory path doesn't exist: " << config.capsules_dir_path_ << std::endl;
        return false;
    }

    std::cout << long_program_desc << std::endl;
    return true;
}

void test(cv::Mat img, int k)
{
    cv::imshow("Input", img);
    cv::waitKey();
    cv::Mat data;
    img.convertTo(data, CV_32F);          // convert to float
    data = data.reshape(1, data.total()); // Flatten, so that there's only one pixel per row

    // do kmeans
    cv::Mat labels, centers;
    cv::kmeans(data, k, labels, cv::TermCriteria(cv::TermCriteria::MAX_ITER, 10, 1.0), 3,
               cv::KMEANS_PP_CENTERS, centers);

    // reshape both to a single row of Vec3f pixels:
    centers = centers.reshape(3, centers.rows);
    data = data.reshape(3, data.rows);

    // replace pixel values with their center value:
    cv::Vec3f *p = data.ptr<cv::Vec3f>();
    for (size_t i = 0; i < data.rows; i++)
    {
        int center_id = labels.at<int>(i);
        p[i] = centers.at<cv::Vec3f>(center_id);
    }

    // back to 2d, and uchar:
    img = data.reshape(3, img.rows);
    img.convertTo(img, CV_8U);

    cv::imshow("Dominant color", img);
    cv::waitKey();

    /////////
    //////////
    int siz = 64;
    cv::Mat cent = centers.reshape(3, centers.rows);
    // make  a horizontal bar of K color patches:
    cv::Mat draw(siz, siz * cent.rows, cent.type(), cv::Scalar::all(0));
    for (int i = 0; i < cent.rows; i++)
    {
        // set the resp. ROI to that value (just fill it):
        draw(cv::Rect(i * siz, 0, siz, siz)) = cent.at<cv::Vec3f>(i, 0);
    }
    draw.convertTo(draw, CV_8U);

    // optional visualization:
    cv::imshow("CENTERS", draw);
    cv::waitKey();
}

int main(int argc, char **argv)
{
    Config config;
    if (!parse_command_line(argc, argv, config))
        return 1;

    cv::Mat input_img;
    try
    {
        input_img = cv::imread(config.image_path_);
    }
    catch (...)
    {
        std::cerr << "Error" << std::endl;
        return 1;
    }

    // CapsulesSolver solver;
    // solver.solve(input_img, config.capsules_dir_path_, config.n_rows_);
    test(input_img, config.n_rows_);
    return 0;
}
