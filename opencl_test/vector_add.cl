__kernel
void vecadd(__global int *A,__global int *B,__global int *C)
{
	//get the work-item's uniqie id
	int idx=get_global_id(0);
	//add the corresponding locations of  A and Band store the result in C
	C[idx]=A[idx]+B[idx];

}