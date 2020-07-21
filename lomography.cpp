/******************************************************************************
 * \brief Application of lomography on an image
 *
 * Lomography filter is used to modify a photograph to improve its aesthetics.
 * This program applies two lomography filters to the supplied photograph.  It
 * computes and applies an LUT to manipulate the red color channel in the photo
 * and then, creates a vignette effect by applying a dark halo to the photo.
 * The program uses a trackbar for each filter to manipulate the parameters.
 *
 * @param photograph as an image file.
 *****************************************************************************/

#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

const std::string keys =	//!< Variable to provide to command line parser
{
	"{help h usage ?	|      | print this message								}"
	"{@filename		    |      | Picture file									}"
};

const std::string lomo_f = "Lomography Filter";   //!< Name of the display window

cv::Mat img,			//!< holds the input image/photo
		result,			//!< contains result from the color filter
		display;		//!< contains result from the vignette filter to be displayed
int s = 10;				//!< used by color filter; value varies from 0 to 20; valid values are from 8 to 20.
int radius = 100;		//!< used by vignette filter; value varies from 1 to 100


/******************************************************************************
 * \brief trackbar_color: Callback function to manipulate color filter.
 *
 * Manipulates the parameters for color filter, computes the LUT corresponding
 * to the supplied parameter and applies it to the red channel.
 *
 * @param None
 * \return None
 *****************************************************************************/

static void trackbar_color(int, void*)
{
	const double exp_e = std::exp(1.0);		//!< Compute the value of e

	// Normalize s within 8 and 20

	s = std::max(s, 8);		//!< Valid values of s are from 8 to 20.

	// Create LUT for color curve effect

	cv::Mat lut(1, 256, CV_8UC1);	//!< LUT to hold color curve effect
	for (int i = 0; i < 256; i++)
	{
		float x = static_cast<float>(i) / 256;
		lut.at<uchar>(i) = cvRound(256 * (1 / (1 + pow(exp_e, -((x - 0.5) / (static_cast<double>(s)/100))))));
	}

	// Split the image channels and apply curve transform only to red channel

	std::vector <cv::Mat> bgr;		//!< Vector to separately hold three color channels
	cv::split(img, bgr);			// Split the color channels
	cv::LUT(bgr[2], lut, bgr[2]);	// Apply LUT to red channel (channel 2)
	merge(bgr, result);				// Merge the color channels into a single BGR image

	cv::imshow(lomo_f, result);		// Display the resulting image
}


/******************************************************************************
 * \brief Apply the vignette filter.
 *
 * Apply the vignette filter by modifying the dark halo circle.  The parameter
 * from trackbar specifies the percentage of radius of the circle that stays
 * bright.  The pixels inside the circle stay bright while those outside are
 * darkened.
 *
 * @param [in] None
 * \return None
 *****************************************************************************/

static void trackbar_halo(int, void*)
{
	// If color filter has not been used, result will be empty.  Copy input
	// image into result.

	if (result.empty())
		img.copyTo(result);

	// Create image for halo dark as a 32-bit floating point 3-channel image.
	// Initialize each pixel in each channel of the image to 0.5.

	cv::Mat halo(img.size(), CV_32FC3, cv::Scalar(0.5, 0.5, 0.5));

	// Create circle based on the supplied radius.  The maximum diameter of the
	// circle will be the smaller of the number of rows and columns in the
	// input image.  Since radius is specified as percentage, divide it by 100
	// to get the fraction, and multiply the max radius by this fraction.
	// Max radius is computed by dividing the max diameter by 2.

	radius = static_cast<int>(std::min(img.cols, img.rows) * (radius/200.0));
	radius = std::max(1, radius);		// Make sure that radius is not zero.

	// Create a circle of computed radius as all white, positioned at center
	// of the original image, and blur it.

	cv::circle(halo, cv::Point(img.cols / 2, img.rows / 2), radius, cv::Scalar(1, 1, 1), -1);
	cv::blur(halo, halo, cv::Size(radius, radius));

	// Convert the result to float to allow multiply by 1 factor

	cv::Mat resultf;
	result.convertTo(resultf, CV_32FC3);

	// Multiply result with halo

	cv::multiply(resultf, halo, resultf);

	// Convert to 8 bits and display the image.

	resultf.convertTo(display, CV_8UC3);
	cv::imshow(lomo_f, display);
}



int main(const int argc, const char** argv)
{
	try
	{
		// Parse the command line arguments

		cv::CommandLineParser parser(argc, argv, keys);
		std::string filename = parser.get<std::string>(0);

		// Read the input file

		parser.about("Lomography v1.0");
		if (parser.has("help") || filename.empty())
		{
			parser.printMessage();
			return (1);
		}

		// Make sure that the parameters are correctly parsed

		if (!parser.check())
		{
			parser.printErrors();
			return (1);
		}

		// Read in the image

		img = cv::imread(filename);
		if (img.empty())
		{
			throw (std::string("Unable to open picture ") + filename);
		}

		// Create the display window and trackbars, and display the image

		cv::namedWindow(lomo_f, cv::WINDOW_AUTOSIZE);
		cv::createTrackbar("s", lomo_f, &s, 20, trackbar_color);
		cv::createTrackbar("radius", lomo_f, &radius, 100, trackbar_halo);
		cv::imshow(lomo_f, img);

		uchar response = cv::waitKey();
		if (response == 'q')
			return (0);
		if (response == 's')
			cv::imwrite("output.jpg", display);

		cv::destroyWindow(lomo_f);
	}
	catch (std::string& str)
	{
		std::cerr << "Error: " << argv[0] << ": " << str << std::endl;
		return (1);
	}
	catch (cv::Exception& e)				// Handle OpenCV exception
	{
		std::cerr << "Error: " << argv[0] << ": " << e.msg << std::endl;
		return (1);
	}

	return (0);
}