#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<string>
#include<CL/cl.h>
#include<fstream>
#include<time.h>
#include "bmputil.h"
#pragma warning( disable : 4996 )
#define SUCCESS 1
#define FAILURE  0
#define HIST_BINS 256
using namespace std;

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

void seriesHistogram(int *image, int height, int width)
{
	clock_t startTime,endTime;
	int histogram[HIST_BINS] = { 0 };
	startTime = clock();
	for (int i = 0; i < height*width; i++)
		histogram[image[i]]++;
	endTime = clock();
	cout << "line Compute Totel Time : " << (double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
}

void chk(cl_int status, const char* cmd) {

	if (status != CL_SUCCESS) {
		printf("%s failed (%d)\n", cmd, status);
		system("pause");
		exit(-1);
	}
}

int main()
{
	int imageHeight;
	int imageWidth;
	const char* inputFile = "input2.bmp";
	const char* outputFile = "";
	int *inputImage = readImage(inputFile, &imageWidth, &imageHeight);
	clock_t startTime, endTime;
	int numPixel = imageHeight*imageWidth;
	int datasize = numPixel * sizeof(int);
	int *outputHistogram = NULL;
	const int histogramSize = HIST_BINS * sizeof(int);
	outputHistogram = (int*)malloc(histogramSize);

	cl_int status;

	//---------------------------------------------
	//SLIP 1:Discover  and initialize the platforms
	//---------------------------------------------
	cl_uint numPlatforms = 0;
	cl_platform_id *platforms = NULL;
	status = clGetPlatformIDs(0, NULL, &numPlatforms);
	chk(status, "GetPlatformIDs");
	platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id)*numPlatforms);
	status = clGetPlatformIDs(numPlatforms, platforms, NULL);

	//---------------------------------------------
	//SLIP 2:Discover and initialize the devices
	//---------------------------------------------
	cl_uint numDevices = 0;
	cl_device_id *devices = NULL;
	status = clGetDeviceIDs(platforms[0],CL_DEVICE_TYPE_GPU,0, NULL, &numDevices);
	chk(status, "GetDeviceIDs");

	devices = (cl_device_id*)malloc(sizeof(cl_device_id)*numDevices);
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
	chk(status, "GetCommandQueue");

	//---------------------------------------------
	//SLIP 5:Create device buffers
	//---------------------------------------------
	cl_mem d_input;
	cl_mem d_output;

	d_input = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, datasize, inputImage, &status);
	d_output = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, histogramSize, outputHistogram, &status);
	/* Initialize the output histogram with zero */
	int zero = 0;
	status = clEnqueueFillBuffer(cmdQueue, d_output, &zero, sizeof(int), 0, histogramSize, 0, NULL, NULL);
	chk(status, "EnqueueFillBuffer");
	if (d_input == NULL || d_output == NULL)
		perror("Error in clCreateBuffer");

	//---------------------------------------------
	//SLIP 6:Create adn complie the program
	//---------------------------------------------
	const char*filename = "histogram.cl";
	string sourceStr;
	status = convertToString(filename, sourceStr);

	const char* source = sourceStr.c_str();
	size_t sourceSize[] = { strlen(source) };
	cl_program program= clCreateProgramWithSource(context, 1, &source, sourceSize, &status);

	status = clBuildProgram(program, numDevices, devices, NULL, NULL, NULL);
	
	if (status != CL_SUCCESS)
	{
		printf("clBuild failed:%d\n", status);
		char tbuf[0x10000];
		clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, 0x10000, tbuf, //print the complier information
			NULL);
		printf("\n%s\n", tbuf);
		system("pause");
		return FAILURE;
	}

	//---------------------------------------------
	//SLIP 7:Create the kernel
	//---------------------------------------------
	cl_kernel kernel = NULL;
	kernel = clCreateKernel(program, "histogram", &status);
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
	chk(status, "CreateKernel");
	
	//---------------------------------------------
	//SLIP 8:Set the kernel arguments
	//---------------------------------------------

	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_input);
	chk(status, "CreateKernel parameter0");
	status = clSetKernelArg(kernel, 1, sizeof(int), &numPixel);
	chk(status, "CreateKernel parameter1");
	status = clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_output);
	chk(status, "CreateKernel parameter2");

	//---------------------------------------------
	//SLIP 9:Configure the work item structure
	//---------------------------------------------
	size_t globalSize[1];
	globalSize[0] = (size_t)1024;
	size_t localSize[1];
	localSize[0] = (size_t)32;
	cl_event prof_event;

	//---------------------------------------------
	//SLIP 10:Enqueue the kernel for excution
	//---------------------------------------------
	seriesHistogram(inputImage, imageHeight, imageWidth);
	startTime = clock();
	status = clEnqueueNDRangeKernel(cmdQueue, kernel, 1, NULL, globalSize, localSize, 0, NULL, &prof_event);
	endTime = clock();
	cout << "GPU Compute Totel Time : " << (double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
	
	//---------------------------------------------
	//SLIP 11:Read result from the buffer 
	//---------------------------------------------
	clEnqueueReadBuffer(cmdQueue, d_output, CL_TRUE, 0, histogramSize, outputHistogram, 0, NULL, NULL);

	//---------------------------------------------
	//SLIP 11:Read result from the buffer 
	//---------------------------------------------
	for (int i = 0; i < HIST_BINS; i++)
		cout << outputHistogram[i] << " ";
	cout << endl;
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
	free(outputHistogram);
	free(platforms);
	free(devices);
	system("pause");
	return 0;
}