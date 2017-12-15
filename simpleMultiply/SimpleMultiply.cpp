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



int main()
{
	// this code executes on the opencl host
	//host data
	const int Ndim = 2;
	const int Mdim = 3;
	const int Pdim = 4;
	int szA = Ndim*Pdim;
	int szB = Mdim*Pdim;
	int szC = Mdim*Ndim;
	float *A = NULL;//input array
	float *B = NULL;//input array
	float *C = NULL;//output array

					//Elements in each array

					//compute the size of the data 

					//allocate soace for input/output data
	A = (float*)malloc(szA*sizeof(float));
	B = (float*)malloc(szB*sizeof(float));
	C = (float*)malloc(szC*sizeof(float));
	//initialize the inputdata
	for (int i = 0; i < szA; i++)
	{
		A[i] = (float)i;

	}
	for (int i = 0; i < szB; i++)
	{
		B[i] = (float)i;

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
	if(cmdQueue == NULL)
		perror("Failed to create commandQueue for device 0.");
	//---------------------------------------------
	//SLIP 5:Create device buffers
	//---------------------------------------------
	cl_mem bufferA;
	cl_mem bufferB;
	cl_mem bufferC;
	bufferA = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, szA * sizeof(float), A, &status);
	bufferB = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, szB * sizeof(float), B, &status);
	bufferC = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR, szC * sizeof(float), C, &status);

	if (bufferA == NULL || bufferB == NULL || bufferC == NULL)
		perror("Error in clCreateBuffer");
	//---------------------------------------------
	//SLIP 6:Write host data to device buffers
	//---------------------------------------------


	//---------------------------------------------
	//SLIP 7:Create adn complie the program
	//---------------------------------------------

	const char* filename = "matrix_mult.cl";
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
	kernel = clCreateKernel(program, "matrix_mult", &status);

	//---------------------------------------------
	//SLIP 9:Set the kernel arguments
	//---------------------------------------------
	status = clSetKernelArg(kernel, 0, sizeof(int), &Ndim);
	status = clSetKernelArg(kernel, 1, sizeof(int), &Mdim);
	status = clSetKernelArg(kernel, 2, sizeof(int), &Pdim);
	status = clSetKernelArg(kernel, 3, sizeof(cl_mem), &bufferA);
	status = clSetKernelArg(kernel, 4, sizeof(cl_mem), &bufferB);
	status = clSetKernelArg(kernel, 5, sizeof(cl_mem), &bufferC);
	if (status)
		cout << "wrong parameter" << endl;
	//---------------------------------------------
	//SLIP 10:Configure the work item structure
	//---------------------------------------------
	size_t globalWorkSize[2];
	globalWorkSize[0] = (size_t)Ndim;
	globalWorkSize[1] = (size_t)Mdim;
	cl_event prof_event;
	//---------------------------------------------
	//SLIP 11:Enqueue the kernel for excution
	//---------------------------------------------
	status = clEnqueueNDRangeKernel(cmdQueue, kernel, 2, NULL, globalWorkSize, NULL, 0, NULL, &prof_event);

	//---------------------------------------------
	//SLIP 112:Enqueue the kernel for excution
	//---------------------------------------------
	clEnqueueReadBuffer(cmdQueue, bufferC, CL_TRUE, 0, sizeof(float)*szC, C, 0, NULL, NULL);
	printf("\nArray A:\n");
	for (int i = 0; i < Ndim; i++) {
		for (int j = 0; j < Pdim; j++)
			printf("%.3f\t", A[i*Pdim + j]);
		printf("\n");
	}
	printf("\nArray B:\n");
	for (int i = 0; i < Pdim; i++) {
		for (int j = 0; j < Mdim; j++)
			printf("%.3f\t", B[i*Mdim + j]);
		printf("\n");
	}
	printf("\nArray C:\n");
	for (int i = 0; i < Ndim; i++) {
		for (int j = 0; j < Mdim; j++)
			printf("%.3f\t", C[i*Mdim + j]);
		printf("\n");
	}

	cout << endl;

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