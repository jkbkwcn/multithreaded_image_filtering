#include <iostream>
#include <stdlib.h>
#include <numeric>
#include <algorithm>
#include <vector>
#include <string>
#include <exception>
#include <memory>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "osapi.h"

#define MAX_THREADS_COUNT 16

using namespace std::chrono_literals;

struct img_dim_t {
	int w;		// Width
	int h;		// Height
	int ch;		// Channels
};

struct kernel_data_t {
	std::vector<int> data;
	int size;
	int div;
};

// Used for padding image	
void imagePadding(img_dim_t img_dim, img_dim_t img_p_dim, int pad, unsigned char *img, unsigned char *img_p) {

	// Copying the image with proper padding - extending existing image with border pixels
	/* 
	Extending top row of the oryginal image to the top:
	1. Copying the first pixel of the oryginal image (see https://en.wikipedia.org/wiki/Kernel_(image_processing) -> Section: "Edge handling") (extending)
	2. Copying the first row (extending)
	3. Copying last last pixel of the oryginal row (extending)
	*/
	unsigned char *row_ptr, *last_p_first_row, *src_row, *last_p_row, *last_p_last_row;
    for (int r = 0; r < pad; r++) {
        row_ptr = img_p + (r * img_p_dim.w * img_dim.ch);
        
        // Upper left - last upper-left pixel
        for (int i = 0; i < pad; i++) {
            memcpy(row_ptr + (i * img_dim.ch), img, img_dim.ch);
		}

        // Center - whole row of the original image
        memcpy(row_ptr + (pad * img_dim.ch), img, img_dim.w * img_dim.ch);
        
        // Upper right - last upper-right pixel
        last_p_first_row = img + (img_dim.w - 1) * img_dim.ch;
        for (int i = 0; i < pad; i++) {
            memcpy(row_ptr + (pad + img_dim.w + i) * img_dim.ch, last_p_first_row, img_dim.ch);
		}
    }

    // Copying the image - whole original image, center of padded imaged
	/* 
	Copying the image:
	1. Copying the first pixel of the given row of the oryginal image (extending)
	2. Copying the first row
	3. Copying the last pixel of the given row of the oryginal image (extending)
	*/
    for (int r = 0; r < img_dim.h; r++) {
        row_ptr = img_p + ((r + pad) * img_p_dim.w * img_dim.ch);
        src_row = img + (r * img_dim.w * img_dim.ch);
        
        // Left border - extending the left border pixel
        for (int i = 0; i < pad; i++) {
            memcpy(row_ptr + (i * img_dim.ch), src_row, img_dim.ch);
		}
        
        // Whole row of the original image
        memcpy(row_ptr + (pad * img_dim.ch), src_row, img_dim.w * img_dim.ch);
        
        // Right border - extending the right border pixel
        last_p_row = src_row + (img_dim.w - 1) * img_dim.ch;
        for (int i = 0; i < pad; i++) {
            memcpy(row_ptr + (pad + img_dim.w + i) * img_dim.ch, last_p_row, img_dim.ch);
		}
    }

    // Extending bottom row of the oryginal image to the bottom; Similarly to extending the first rows
    unsigned char* last_row_src = img + (img_dim.h - 1) * img_dim.w * img_dim.ch;
    for (int r = 0; r < pad; r++) {
        row_ptr = img_p + ((img_dim.h + pad + r) * img_p_dim.w * img_dim.ch);
        
        // Lower left - last lower-left pixel
        for (int i = 0; i < pad; i++) {
            memcpy(row_ptr + (i * img_dim.ch), last_row_src, img_dim.ch);
		}
        
        // Center - whole row of the original image
        memcpy(row_ptr + (pad * img_dim.ch), last_row_src, img_dim.w * img_dim.ch);
        
        // Lower right - last lower-right pixel
        last_p_last_row = last_row_src + (img_dim.w - 1) * img_dim.ch;
        for (int i = 0; i < pad; i++) {
            memcpy(row_ptr + (pad + img_dim.w + i) * img_dim.ch, last_p_last_row, img_dim.ch);
		}
    }
}

class WorkerThread : public osapi::Thread
{
public:
	WorkerThread(const unsigned char *imgIn, unsigned char *imgOut, const kernel_data_t &kernelData, img_dim_t imgInDim, img_dim_t imgOutDim, int startRow, int endRow) : Thread(0, 128, osapi::JOINABLE), mImgIn{imgIn}, mImgOut{imgOut}, mKernelData{kernelData}, mImgInDim{imgInDim}, mImgOutDim{imgOutDim}, mStartRow{startRow}, mEndRow{endRow} {;}

private:
	const unsigned char *mImgIn;
	unsigned char *mImgOut;
	const kernel_data_t &mKernelData;
	img_dim_t mImgInDim;
	img_dim_t mImgOutDim;
	int mStartRow;
	int mEndRow;

	void job(void) override {

		// Kernel data as pointer to iterate over memory
		const int* kernel = mKernelData.data.data();

		// Sums (convolution), temporary kernel data values
		int sum_r, sum_g, sum_b;
		int r, g, b;
		int k_val;

		// Pointers; ensure that input image i read-only
		const unsigned char *px_p; 
		unsigned char *px_f;

		int pad = mKernelData.size;
		
		// Iteration over rows and columns of the resultant image (indexing over output image)
		for (int y = mStartRow; y < mEndRow; y++) {
			for (int x = 0; x < mImgOutDim.w; x++) {
				
				// Termporary sums - convoltion results
				sum_r = 0;
				sum_g = 0;
				sum_b = 0;

				// Iteration over image pixels with the kernel
				for (int ky = 0; ky < mKernelData.size; ky++) {
					for (int kx = 0; kx < mKernelData.size; kx++) {

						// Value of pixel (indexing over padded image)
						px_p = mImgIn + ((y + ky) * mImgInDim.w + (x + kx)) * mImgInDim.ch;
						k_val = kernel[ky * mKernelData.size + kx];

						// Convolution - temporary sums
						sum_r += k_val * px_p[0];
						sum_g += k_val * px_p[1];
						sum_b += k_val * px_p[2];
					}
				}

				// Writing results of convolution, ensure proper clamping
				px_f = mImgOut + (y * mImgOutDim.w + x) * mImgOutDim.ch;
				px_f[0] = (uint8_t)std::clamp(sum_r / mKernelData.div, 0, 255);
				px_f[1] = (uint8_t)std::clamp(sum_g / mKernelData.div, 0, 255);
				px_f[2] = (uint8_t)std::clamp(sum_b / mKernelData.div, 0, 255);
			}
		}
	}
};


int main() {

	// ----------------------------------------------------------------------------
	// ------------------------------- LOADING IMAGE ------------------------------
	// ----------------------------------------------------------------------------
	
	std::string input_path = "..\\img\\image.jpg";
	std::string input_path_pad = "..\\img\\image_pad.jpg";
	std::string output_path;
	
	// Imgage dimensions
	img_dim_t img_dim;

	/*
	The image is loaded in the structure with the following order,
	marked as <COLOR>_<ROW><COLUMN>:
	R_00 - G_00 - B_00 - R_01 - G_01 - B_01 - ...
	*/
 	unsigned char *img = stbi_load(input_path.c_str(), &img_dim.w, &img_dim.h, &img_dim.ch, 0);
 	
	if(img == NULL) {
 		printf("Error in loading the image\n");
 		exit(1);
 	}
	
	printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", img_dim.w, img_dim.h, img_dim.ch);

	// Kernel declaration - filtering an image - as 1D table of type signed int (kernel values can be negative)
	kernel_data_t kernel;

	// ----------------------------------------------------------------------------
	// ------------------------------- FILTER CHOICE ------------------------------
	// ----------------------------------------------------------------------------

	int choice;

    printf("Choose filter:\n");
    printf("1. Blur\n");
    printf("2. Gaussian blur\n");
    printf("3. Sharpen\n");
    printf("4. Edge detection\n");
	printf("5. Blur 9x9\n");
    printf("Choice: ");
    scanf("%d", &choice);

	switch (choice) {
        case 1: // Blur
            kernel.data = { 
				1, 1, 1, 
				1, 1, 1, 
				1, 1, 1 
			};
            kernel.div = 9;
			kernel.size = 3;
			output_path = "..\\img\\image_blur.jpg";
            break;
        case 2: // Gaussian blur
            kernel.data = { 
				1, 2, 1, 
				2, 4, 2, 
				1, 2, 1 
			};
            kernel.div = 16;
			kernel.size = 3;
			output_path = "..\\img\\image_gaussian_blur.jpg";
            break;
        case 3: // Sharpen
            kernel.data = { 
				0, -1, 0, 
				-1, 5, -1, 
				0, -1, 0 
			};
            kernel.div = 1;
			kernel.size = 3;
			output_path = "..\\img\\image_sharpen.jpg";
            break;
        case 4: // Edge detection
            kernel.data = { 
				0, -1, 0, 
				-1, 4, -1, 
				0, -1, 0 
			};
            kernel.div = 1;
			kernel.size = 3;
			output_path = "..\\img\\image_edge_detection.jpg";
            break;
		case 5: // Blur 9x9
            kernel.data = {
				1, 1, 1, 1, 1, 1, 1, 1, 1,   
				1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1, 1,   
				1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1, 1,   
				1, 1, 1, 1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 1, 1, 1, 1
			};
            kernel.div = 81;
			kernel.size = 9;
			output_path = "..\\img\\image_blur_9x9.jpg";
            break;
        default:
            printf("Invalid choice. Closing...\n");
			free(img);
            return 1;
    }

	// ----------------------------------------------------------------------------
	// --------------------------------- PADDING ----------------------------------
	// ----------------------------------------------------------------------------

	printf("Padding image...\n");

	// Padding is needed to keep original image shape
	// Padded image dimensions
	img_dim_t img_p_dim;

    int img_pad = kernel.size / 2;
    img_p_dim.w = img_dim.w + 2 * img_pad;
    img_p_dim.h = img_dim.h + 2 * img_pad;
	img_p_dim.ch = img_dim.ch;

	// Allocate memory for padded image
    size_t img_p_size = img_p_dim.w * img_p_dim.h * img_p_dim.ch;
    unsigned char *img_p = (unsigned char*)malloc(img_p_size);

	// Allocate memory for fitered image
	size_t img_size = img_dim.w * img_dim.h * img_dim.ch;
	unsigned char *img_filtered = (unsigned char*)malloc(img_size);

	// Image padding
	imagePadding(img_dim, img_p_dim, img_pad, img, img_p);

	// ----------------------------------------------------------------------------
	// -------------------------------- FILTRATION --------------------------------
	// --------------------- EXECUTION TIME VS THREADS NUMBER ---------------------
	// ----------------------------------------------------------------------------

	printf("Filtering image...\n");

	// Declaration needed for time measurements
	std::chrono::duration<double, std::milli> speedup_reference{0};

	std::vector<std::unique_ptr<WorkerThread>> threads;

	for(int thread_count = 1; thread_count <= MAX_THREADS_COUNT; thread_count++)
	{
		// Save start time
		auto startTime = std::chrono::system_clock::now();

		// Divide work - divide image into equal rows
		int rows = img_dim.h / thread_count;
		int start_row, end_row;

		// Threads creation
		for(int count = 0; count < thread_count; count++) {
			start_row = rows * count;
			// Ensure filtration of last rows (integer division)
			if(rows * (count + 2) > img_dim.h) {
				end_row = img_dim.h;
			} else {
				end_row = rows * (count + 1);
			}
			threads.emplace_back(std::make_unique<WorkerThread>(img_p, img_filtered, kernel, img_p_dim, img_dim, start_row, end_row));
			threads.back()->run();
		}

		// Wait until all threads finish
		for(auto& t : threads) {
			while(!t->join(1000)) {}
		}

		// Save end time
		auto endTime = std::chrono::system_clock::now();

		// Reference for speedup calculation
		if(1 == thread_count) {
			speedup_reference = endTime - startTime;
		}

		// Print results
		auto duration = endTime - startTime;

		// Speedup calculation
		auto speedup = std::chrono::duration <double, std::milli>(speedup_reference).count() / std::chrono::duration <double, std::milli>(duration).count();

		printf("----------------------------\n");
		printf("Threads number: %d\n", thread_count);
		printf("Time duration:  %.3f [ms]\n", std::chrono::duration <double, std::milli>(duration).count());
		printf("Speedup: %.3f\n", speedup);

		// Clear thread vector
		threads.clear();
	}
	
	// ----------------------------------------------------------------------------
	// -------------------- WRITING OUTPUT TO VALIDATE REASULT --------------------
	// ----------------------------------------------------------------------------
	
	printf("Writing output image...\n");

	// Writing filtered image
	int err = stbi_write_jpg(output_path.c_str(), img_dim.w, img_dim.h, img_dim.ch, img_filtered, 100);

	printf("\n\n----------------------------\n");

	if(err) {
		printf("Image has been writen correctly.");
	} else {
		printf("Error while writing image. Error code: %d\n", err);
	}

	// Free alocated memory
	free(img);
	free(img_p);
	free(img_filtered);
	
	return 0;
}
