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

//oepncl kernel to perform  an element-wise
//add if two arrays
const char*programSource =
"__kernel							\n"
"void vecadd(__global int *A)		";


int main()
{
	// this code executes on the opencl host
	//host data
	int *A = NULL;//input array
	int *B = NULL;//input array
	int *C = NULL;//output array

	//Elements in each array
	const int  elements = 10;
	//compute the size of the data 
	size_t datasize = sizeof(int)*elements;
	//allocate soace for input/output data
	A = (int*)malloc(datasize);
	B = (int*)malloc(datasize);
	C = (int*)malloc(datasize);
	//initialize the inputdata
	for (int i = 0; i < elements; i++)
	{
		A[i] = i;
		B[i] = i;
	}

	//use this to check  the output of each API call
	cl_int status;

	//---------------------------------------------
	//SLIP 1:Discover  and initialize the platforms
	//---------------------------------------------
	cl_uint numPlatforms = 0;
	cl_platform_id *platforms = NULL;
	//
	status = clGetPlatformIDs(0, NULL, &numPlatforms);
	platforms = (cl_platform_id *)malloc(numPlatforms * sizeof(cl_platform_id));
	status = clGetPlatformIDs(numPlatforms, platforms, NULL);
	
	//---------------------------------------------
	//SLIP 2:Discover and initialize the devices
	//---------------------------------------------
	cl_uint numDevices = 0;
	cl_device_id *devices = NULL;

	status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);

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

	//---------------------------------------------
	//SLIP 5:Create device buffers
	//---------------------------------------------
	cl_mem bufferA;
	cl_mem bufferB;
	cl_mem bufferC;
	bufferA = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, &status);
	bufferB = clCreateBuffer(context, CL_MEM_READ_ONLY, datasize, NULL, &status);
	bufferC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, datasize, NULL, &status);

	//---------------------------------------------
	//SLIP 6:Write host data to device buffers
	//---------------------------------------------

	status = clEnqueueWriteBuffer(cmdQueue, bufferA, CL_FALSE, 0, datasize, A, 0, NULL, NULL);
	status = clEnqueueWriteBuffer(cmdQueue, bufferB, CL_FALSE, 0, datasize, B, 0, NULL, NULL);

	//---------------------------------------------
	//SLIP 7:Create adn complie the program
	//---------------------------------------------
	
	const char* filename = "vector_add.cl";
	string sourceStr;
	status = convertToString(filename, sourceStr);
	cout << sourceStr << endl;
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = { strlen(source) };
	cl_program program = clCreateProgramWithSource(context, 1, &source, sourceSize, &status);

	status = clBuildProgram(program, numDevices, devices, NULL, NULL, NULL);

	//---------------------------------------------
	//SLIP 8:Create the kernel
	//---------------------------------------------

	cl_kernel kernel = NULL;
	kernel = clCreateKernel(program, "vecadd", &status);

	//---------------------------------------------
	//SLIP 9:Set the kernel arguments
	//---------------------------------------------
	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufferA);
	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufferB);
	status = clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufferC);
	//---------------------------------------------
	//SLIP 10:Configure the work item structure
	//---------------------------------------------
	size_t globalWorkSize[1];
	globalWorkSize[0] = elements;

	//---------------------------------------------
	//SLIP 11:Enqueue the kernel for excution
	//---------------------------------------------
	status = clEnqueueNDRangeKernel(cmdQueue, kernel, 1, NULL, globalWorkSize, NULL, 0, NULL, NULL);
	
	//---------------------------------------------
	//SLIP 112:Enqueue the kernel for excution
	//---------------------------------------------
	clEnqueueReadBuffer(cmdQueue, bufferC, CL_TRUE, 0, datasize, C, 0, NULL, NULL);
	bool result = true;
	for (int i = 0; i < elements; i++)
	{
		cout << C[i] << endl;
	
		if (C[i] != i + i) {
			result = false;
			break;
			//cout << C[i] << endl;
		}
	}

	if (result) {
		printf("correct\n");
	}
	else {
		printf("incorrect\n");
	}

	//---------------------------------------------
	//SLIP 13:Release Opencl resources
	//---------------------------------------------
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(cmdQueue);
	clReleaseMemObject(bufferA);
	clReleaseMemObject(bufferB);
	clReleaseMemObject(bufferC);
	clReleaseContext(context);
	free(A);
	free(B);
	free(C);
	free(platforms);
	free(devices);
	system("pause");
	return 0;
}