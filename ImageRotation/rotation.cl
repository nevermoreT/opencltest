__kernel void imageRotation(int w, int h, float sin, float cos, __global float* src_data, __global float* dest_data)
{
	//坐标系原点在左下角，图像的存储从左上开始，行优先存储。
	const int ix = get_global_id(0);
	const int iy = get_global_id(1);
	float x0 = w / 2.0f;
	float y0 = h / 2.0f;
	float xoff = ix - x0;
	float yoff = iy - y0;
	int xpos = (int)(xoff * cos + yoff * sin + x0);
	int ypos = (int)(yoff * cos - xoff * sin + y0);
	if ((xpos >= 0) && (xpos < w) && (ypos >= 0) && (ypos < h))
	{
		dest_data[iy*w + ix] = src_data[ypos*w + xpos];
	}
}
