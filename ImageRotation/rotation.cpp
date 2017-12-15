//this program implements a vector addition using OpenCL
//System includes
#include<stdio.h>
#include<stdlib.h>
#include<iostream>
#include <string>
#include <string.h>
#include<fstream>
//oepncl includes
#include<CL/cl.h>

#pragma warning( disable : 4996 )
#define SUCCESS 0
#define FAILURE 1
using namespace std;
typedef unsigned char uchar;


void storeImage(float *imageOut, const char *filename, int rows, int cols,
	const char* refFilename) {

	FILE *ifp, *ofp;
	unsigned char tmp;
	int offset;
	unsigned char *buffer;
	int i, j;

	int bytes;

	int height, width;

	ifp = fopen(refFilename, "rb");
	if (ifp == NULL) {
		perror(filename);
		exit(-1);
	}

	fseek(ifp, 10, SEEK_SET);
	fread(&offset, 4, 1, ifp);

	fseek(ifp, 18, SEEK_SET);
	fread(&width, 4, 1, ifp);
	fread(&height, 4, 1, ifp);

	fseek(ifp, 0, SEEK_SET);

	buffer = (unsigned char *)malloc(offset);
	if (buffer == NULL) {
		perror("malloc");
		exit(-1);
	}

	fread(buffer, 1, offset, ifp);

	printf("Writing output image to %s\n", filename);
	ofp = fopen(filename, "wb");
	if (ofp == NULL) {
		perror("opening output file");
		exit(-1);
	}
	bytes = fwrite(buffer, 1, offset, ofp);
	if (bytes != offset) {
		printf("error writing header!\n");
		exit(-1);
	}

	// NOTE bmp formats store data in reverse raster order (see comment in
	// readImage function), so we need to flip it upside down here.  
	int mod = width % 4;
	if (mod != 0) {
		mod = 4 - mod;
	}
	//   printf("mod = %d\n", mod);
	for (i = height - 1; i >= 0; i--) {
		for (j = 0; j < width; j++) {
			tmp = (unsigned char)imageOut[i*cols + j];
			fwrite(&tmp, sizeof(char), 1, ofp);
		}
		// In bmp format, rows must be a multiple of 4-bytes.  
		// So if we're not at a multiple of 4, add junk padding.
		for (j = 0; j < mod; j++) {
			fwrite(&tmp, sizeof(char), 1, ofp);
		}
	}

	fclose(ofp);
	fclose(ifp);

	free(buffer);
}

/*
* Read bmp image and convert to byte array. Also output the width and height
*/
float* readImage(const char *filename, int* widthOut, int* heightOut) {

	uchar* imageData;

	int height, width;
	uchar tmp;
	int offset;
	int i, j;

	printf("Reading input image from %s\n", filename);
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) {
		perror(filename);
		exit(-1);
	}

	fseek(fp, 10, SEEK_SET);
	fread(&offset, 4, 1, fp);

	fseek(fp, 18, SEEK_SET);
	fread(&width, 4, 1, fp);
	fread(&height, 4, 1, fp);

	printf("width = %d\n", width);
	printf("height = %d\n", height);

	*widthOut = width;
	*heightOut = height;

	imageData = (uchar*)malloc(width*height);
	if (imageData == NULL) {
		perror("malloc");
		exit(-1);
	}

	fseek(fp, offset, SEEK_SET);
	fflush(NULL);

	int mod = width % 4;
	if (mod != 0) {
		mod = 4 - mod;
	}

	// NOTE bitmaps are stored in upside-down raster order.  So we begin
	// reading from the bottom left pixel, then going from left-to-right, 
	// read from the bottom to the top of the image.  For image analysis, 
	// we want the image to be right-side up, so we'll modify it here.

	// First we read the image in upside-down

	// Read in the actual image
	for (i = 0; i < height; i++) {

		// add actual data to the image
		for (j = 0; j < width; j++) {
			fread(&tmp, sizeof(char), 1, fp);
			imageData[i*width + j] = tmp;
		}
		// For the bmp format, each row has to be a multiple of 4, 
		// so I need to read in the junk data and throw it away
		for (j = 0; j < mod; j++) {
			fread(&tmp, sizeof(char), 1, fp);
		}
	}

	// Then we flip it over
	int flipRow;
	for (i = 0; i < height / 2; i++) {
		flipRow = height - (i + 1);
		for (j = 0; j < width; j++) {
			tmp = imageData[i*width + j];
			imageData[i*width + j] = imageData[flipRow*width + j];
			imageData[flipRow*width + j] = tmp;
		}
	}

	fclose(fp);

	// Input image on the host
	float* floatImage = NULL;
	floatImage = (float*)malloc(sizeof(float)*width*height);
	if (floatImage == NULL) {
		perror("malloc");
		exit(-1);
	}

	// Convert the BMP image to float (not required)
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			floatImage[i*width + j] = (float)imageData[i*width + j];
		}
	}

	free(imageData);
	return floatImage;
}
















/* convert the kernel file into a string */
int convertToString(const char *filename, std::string& s)
{
	size_t size;
	char*  str;
	std::fstream f(filename, (std::fstream::in | std::fstream::binary));

	if (f.is_open())
	{
		size_t fileSize;
		f.seekg(0, std::fstream::end);
		size = fileSize = (size_t)f.tellg();
		f.seekg(0, std::fstream::beg);
		str = new char[size + 1];
		if (!str)
		{
			f.close();
			return 0;
		}

		f.read(str, fileSize);
		f.close();
		str[size] = '\0';
		s = str;
		delete[] str;
		return 0;
	}
	cout << "Error: failed to open file\n:" << filename << endl;
	return FAILURE;
}



int main()
{
	

	// Set the image rotation (in degrees)
	float theta = 3.14159 / 6;
	float cos_theta = cosf(theta);
	float sin_theta = sinf(theta);
	printf("theta = %f (cos theta = %f, sin theta = %f)\n", theta, cos_theta,
		sin_theta);


	// Rows and columns in the input image
	int imageHeight;
	int imageWidth;

	const char* inputFile = "input.bmp";
	const char* outputFile = "output.bmp";

	
	// Homegrown function to read a BMP from file
	float* inputImage = readImage(inputFile, &imageWidth,&imageHeight);

	// Size of the input and output images on the host
	int dataSize = imageHeight*imageWidth * sizeof(float);

	// Output image on the host
	float* outputImage = NULL;
	outputImage = (float*)malloc(dataSize);
	float* refImage = NULL;
	refImage = (float*)malloc(dataSize);
	//use this to check  the output of each API call
	cl_int status;

	//---------------------------------------------
	//SLIP 1:Discover  and initialize the platforms
	//---------------------------------------------
	cl_uint numPlatforms = 0;
	cl_platform_id *platforms = NULL;
	//
	status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (status != CL_SUCCESS)
	{
		perror("Error:Getting platforms\n");
		return FAILURE;
	}

	platforms = (cl_platform_id *)malloc(numPlatforms * sizeof(cl_platform_id));

	status = clGetPlatformIDs(numPlatforms, platforms, NULL);

	//---------------------------------------------
	//SLIP 2:Discover and initialize the devices
	//---------------------------------------------
	cl_uint numDevices = 0;
	cl_device_id *devices = NULL;

	status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
	if (status != CL_SUCCESS)
	{
		perror("Error:Getting Devices\n");
		return FAILURE;
	}


	devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
	status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);

	//---------------------------------------------
	//SLIP 3:Create a context
	//---------------------------------------------
	cl_context context = NULL;
	context = clCreateContext(NULL, numDevices, devices, NULL, NULL, &status);

	//---------------------------------------------
	//SLIP 4:Create a command queue
	//---------------------------------------------
	cl_command_queue cmdQueue;
	cmdQueue = clCreateCommandQueue(context, devices[0], 0, &status);
	if (cmdQueue == NULL)
		perror("Failed to create commandQueue for device 0.");
	//---------------------------------------------
	//SLIP 5:Create device buffers
	//---------------------------------------------
	cl_mem d_input;
	cl_mem d_output;
	
	d_input = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, dataSize, inputImage, &status);
	
	d_output = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, dataSize, outputImage, &status);

	if (d_input == NULL || d_output == NULL)
		perror("Error in clCreateBuffer");
	//---------------------------------------------
	//SLIP 6:Write host data to device buffers
	//---------------------------------------------


	//---------------------------------------------
	//SLIP 7:Create adn complie the program
	//---------------------------------------------

	const char* filename = "rotation.cl";
	string sourceStr;
	status = convertToString(filename, sourceStr);
	//cout << sourceStr << endl;
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = { strlen(source) };
	cl_program program = clCreateProgramWithSource(context, 1, &source, sourceSize, &status);

	status = clBuildProgram(program, numDevices, devices, NULL, NULL, NULL);
	if (status != CL_SUCCESS)
	{
		printf("clBuild failed:%d\n", status);
		char tbuf[0x10000];
		clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, 0x10000, tbuf,
			NULL);
		printf("\n%s\n", tbuf);
		return FAILURE;
	}
	//---------------------------------------------
	//SLIP 8:Create the kernel
	//---------------------------------------------

	cl_kernel kernel = NULL;
	kernel = clCreateKernel(program, "imageRotation", &status);
	if (status)
		switch (status)
		{
		case CL_INVALID_PROGRAM:cout << "CL_INVALID_PROGRAM" << endl; break;
		case CL_INVALID_PROGRAM_EXECUTABLE:cout << "CL_INVALID_PROGRAM_EXECUTABLE" << endl; break;
		case CL_INVALID_KERNEL_NAME:cout << "CL_INVALID_KERNEL_NAME" << endl; break;
		case CL_INVALID_KERNEL_DEFINITION:cout << "CL_INVALID_KERNEL_DEFINITION" << endl; break;
		case CL_INVALID_VALUE:cout << "CL_INVALID_VALUE" << endl; break;
		case CL_OUT_OF_RESOURCES:cout << "CL_INVALID_VALUE" << endl; break;
		case CL_OUT_OF_HOST_MEMORY:cout << "CL_INVALID_VALUE" << endl; break;
		}
		
	//---------------------------------------------
	//SLIP 9:Set the kernel arguments
	//---------------------------------------------
	status = clSetKernelArg(kernel, 0, sizeof(int), &imageWidth);
	if (status)
		cout << "wrong parameter0" << endl;
	status = clSetKernelArg(kernel, 1, sizeof(int), &imageHeight);
	if (status)
		cout << "wrong parameter1" << endl;
	status = clSetKernelArg(kernel, 2, sizeof(float), &sin_theta);
	if (status)
		cout << "wrong parameter2" << endl;
	status = clSetKernelArg(kernel, 3, sizeof(float), &cos_theta);
	if (status)
		cout << "wrong parameter3" << endl;
	status = clSetKernelArg(kernel, 4, sizeof(cl_mem), &d_input);
	if (status)
		cout << "wrong parameter4" << endl;
	status = clSetKernelArg(kernel, 5, sizeof(cl_mem), &d_output);
	if (status)
		cout << "wrong parameter5" << endl;
	//---------------------------------------------
	//SLIP 10:Configure the work item structure
	//---------------------------------------------
	size_t globalWorkSize[2];
	globalWorkSize[0] = (size_t)imageWidth;
	globalWorkSize[1] = (size_t)imageHeight;
	cl_event prof_event;
	//---------------------------------------------
	//SLIP 11:Enqueue the kernel for excution
	//---------------------------------------------
	status = clEnqueueNDRangeKernel(cmdQueue, kernel, 2, NULL, globalWorkSize, NULL, 0, NULL, &prof_event);

	//---------------------------------------------
	//SLIP 112:Enqueue the kernel for excution
	//---------------------------------------------
	clEnqueueReadBuffer(cmdQueue, d_output, CL_TRUE, 0, dataSize, outputImage, 0, NULL, NULL);
	
	storeImage(outputImage, outputFile, imageHeight, imageWidth, inputFile);
	//---------------------------------------------
	//SLIP 13:Release Opencl resources
	//---------------------------------------------
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(cmdQueue);
	clReleaseMemObject(d_input);
	clReleaseMemObject(d_output);
	clReleaseContext(context);
	free(inputImage);
	free(outputImage);
	free(platforms);
	free(devices);
	system("pause");
	return 0;
}