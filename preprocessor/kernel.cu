
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <algorithm>
#define inf 85000

//	const int N = 1520;
//	const int M = 1880;
//	int S[N*M] = {0};
//	float D[N*M] = {0};
    int *dev_S = 0;
    int *dev_P = 0;
    int *dev_P2 = 0;
    float *dev_D = 0;

__global__ void initializationKernel(int * S, int * P, int * P2, int N, int M){
	int col = blockDim.x * blockIdx.x + threadIdx.x;
	int row = blockDim.y * blockIdx.y + threadIdx.y;
	if(col<M && row<N){
		if (S[row*M+col]==1){
			P[row*M+col]=col;
			P[N*M+row*M+col]=row;
			P2[row*M+col]=col;
			P2[N*M+row*M+col]=row;
		} else{
			P[row*M+col]=inf;
			P[N*M+row*M+col]=inf;
			P2[row*M+col]=inf;
			P2[N*M+row*M+col]=inf;
		}
	}
}

__global__ void distanceKernel(float * D, int * P, int N, int M){
	int col = blockDim.x * blockIdx.x + threadIdx.x;
	int row = blockDim.y * blockIdx.y + threadIdx.y;
	if(col<M && row<N){
		D[row*M+col]=sqrtf((P[row*M+col]-col)*(P[row*M+col]-col)+(P[N*M+row*M+col]-row)*(P[N*M+row*M+col]-row));
	}
}

__global__ void copy1ColKernel(int * dev_P, int * dev_P2, int N, int M, int col){
	int row = blockDim.x * blockIdx.x + threadIdx.x;
	//int row = blockDim.x*threadIdx.y + threadIdx.x;
	if(row<N){
		dev_P2[row*M+col]=dev_P[row*M+col];
		dev_P2[N*M+row*M+col]=dev_P[N*M+row*M+col];
	}
}

__global__ void copyColKernel(int * dev_P, int * dev_P2, int N, int M){
	int col = blockDim.x * blockIdx.x + threadIdx.x;
	int row = blockDim.y * blockIdx.y + threadIdx.y;
	if(col<M && row<N){
		if(col%2==0){
			dev_P[row*M+col]=dev_P2[row*M+col];
			dev_P[N*M+row*M+col]=dev_P2[N*M+row*M+col];
		} else{
			dev_P2[row*M+col]=dev_P[row*M+col];
			dev_P2[N*M+row*M+col]=dev_P[N*M+row*M+col];
		}
	}
}

__global__ void copy1RowKernel(int * dev_P, int * dev_P2, int N, int M, int row){
	int col = blockDim.x * blockIdx.x + threadIdx.x;
	//int col = blockDim.x*threadIdx.y + threadIdx.x;
	if(col<M){
		dev_P2[row*M+col]=dev_P[row*M+col];
		dev_P2[N*M+row*M+col]=dev_P[N*M+row*M+col];
	}
}

__global__ void copyRowKernel(int * dev_P, int * dev_P2, int N, int M){
	int col = blockDim.x * blockIdx.x + threadIdx.x;
	int row = blockDim.y * blockIdx.y + threadIdx.y;
	if(col<M && row<N){
		if(row%2==0){
			dev_P[row*M+col]=dev_P2[row*M+col];
			dev_P[N*M+row*M+col]=dev_P2[N*M+row*M+col];
		} else{
			dev_P2[row*M+col]=dev_P[row*M+col];
			dev_P2[N*M+row*M+col]=dev_P[N*M+row*M+col];
		}
	}
}

__global__ void propagation1aKernel(int * dev_P, int * dev_P2, int N, int M, int col){
	int row = blockDim.x * blockIdx.x + threadIdx.x;
	//int row =blockDim.x*threadIdx.y + threadIdx.x;
	int L[4];
	int Lmin;
	int p;
	if(row<N){
		L[0] = (dev_P[row*M+col]-col)*(dev_P[row*M+col]-col) + (dev_P[N*M+row*M+col]-row)*(dev_P[N*M+row*M+col]-row);
		Lmin = L[0];
		p = 0;
		L[2] = (dev_P[row*M+col-1]-col)*(dev_P[row*M+col-1]-col) + (dev_P[N*M+row*M+col-1]-row)*(dev_P[N*M+row*M+col-1]-row);
		if (L[2]<Lmin){
			Lmin = L[2];
			p = 2;
		}
		if(row>0){
			L[1] = (dev_P[(row-1)*M+(col-1)]-col)*(dev_P[(row-1)*M+(col-1)]-col) + (dev_P[N*M+(row-1)*M+(col-1)]-row)*(dev_P[N*M+(row-1)*M+(col-1)]-row);
			if (L[1]<Lmin){
				Lmin = L[1];
				p = 1;
			}
		}
		if(row<(N-1)){
			L[3] = (dev_P[(row+1)*M+(col-1)]-col)*(dev_P[(row+1)*M+(col-1)]-col) + (dev_P[N*M+(row+1)*M+(col-1)]-row)*(dev_P[N*M+(row+1)*M+(col-1)]-row);
			if (L[3]<Lmin){
				p = 3;
				Lmin = L[3];
			}
		}
		switch (p){
			case 0:
				dev_P2[row*M+col] = dev_P[row*M+col];
				dev_P2[N*M+row*M+col] = dev_P[N*M+row*M+col];
				break;
			case 1:
				dev_P2[row*M+col] = dev_P[(row-1)*M+(col-1)];
				dev_P2[N*M+row*M+col] = dev_P[N*M+(row-1)*M+(col-1)];
				break;
			case 2:
				dev_P2[row*M+col] = dev_P[row*M+col-1];
				dev_P2[N*M+row*M+col] = dev_P[N*M+row*M+col-1];
				break;
			case 3:
				dev_P2[row*M+col] = dev_P[(row+1)*M+(col-1)];
				dev_P2[N*M+row*M+col] = dev_P[N*M+(row+1)*M+(col-1)];
				break;
		}
	}
}
__global__ void propagation1bKernel(int * dev_P, int * dev_P2, int N, int M, int col){
	int row = blockDim.x * blockIdx.x + threadIdx.x;
	//int row =blockDim.x*threadIdx.y + threadIdx.x;
	int L[4];
	int Lmin;
	int p;
	if(row<N){
		col++;	
		if(col<M){
			L[0] = (dev_P2[row*M+col]-col)*(dev_P2[row*M+col]-col) + (dev_P2[N*M+row*M+col]-row)*(dev_P2[N*M+row*M+col]-row);
			Lmin = L[0];
			p = 0;
			L[2] = (dev_P2[row*M+col-1]-col)*(dev_P2[row*M+col-1]-col) + (dev_P2[N*M+row*M+col-1]-row)*(dev_P2[N*M+row*M+col-1]-row);
			if (L[2]<Lmin){
				Lmin = L[2];
				p = 2;
			}
			if(row>0){
				L[1] = (dev_P2[(row-1)*M+(col-1)]-col)*(dev_P2[(row-1)*M+(col-1)]-col) + (dev_P2[N*M+(row-1)*M+(col-1)]-row)*(dev_P2[N*M+(row-1)*M+(col-1)]-row);
				if (L[1]<Lmin){
					Lmin = L[1];
					p = 1;
				}
			}
			if(row<(N-1)){
				L[3] = (dev_P2[(row+1)*M+(col-1)]-col)*(dev_P2[(row+1)*M+(col-1)]-col) + (dev_P2[N*M+(row+1)*M+(col-1)]-row)*(dev_P2[N*M+(row+1)*M+(col-1)]-row);
				if (L[3]<Lmin){
					Lmin = L[3];
					p = 3;
				}
			}
			switch (p){
				case 0:
					dev_P[row*M+col] = dev_P2[row*M+col];
					dev_P[N*M+row*M+col] = dev_P2[N*M+row*M+col];
					break;
				case 1:
					dev_P[row*M+col] = dev_P2[(row-1)*M+(col-1)];
					dev_P[N*M+row*M+col] = dev_P2[N*M+(row-1)*M+(col-1)];
					break;
				case 2:
					dev_P[row*M+col] = dev_P2[row*M+col-1];
					dev_P[N*M+row*M+col] = dev_P2[N*M+row*M+col-1];
					break;
				case 3:
					dev_P[row*M+col] = dev_P2[(row+1)*M+(col-1)];
					dev_P[N*M+row*M+col] = dev_P2[N*M+(row+1)*M+(col-1)];
					break;
			}
		}
	}
}

__global__ void propagation2aKernel(int * dev_P, int * dev_P2, int N, int M, int col){
	int row = blockDim.x * blockIdx.x + threadIdx.x;
	//int row = blockDim.x*threadIdx.y + threadIdx.x;
	int L[4];
	int Lmin;
	int p;
	if(row<N){
		L[0] = (dev_P2[row*M+col]-col)*(dev_P2[row*M+col]-col) + (dev_P2[N*M+row*M+col]-row)*(dev_P2[N*M+row*M+col]-row);
		Lmin = L[0];
		p = 0;
		L[2] = (dev_P2[row*M+col+1]-col)*(dev_P2[row*M+col+1]-col) + (dev_P2[N*M+row*M+col+1]-row)*(dev_P2[N*M+row*M+col+1]-row);
		if (L[2]<Lmin){
			Lmin = L[2];
			p = 2;
		}
		if(row>0){
			L[1] = (dev_P2[(row-1)*M+(col+1)]-col)*(dev_P2[(row-1)*M+(col+1)]-col) + (dev_P2[N*M+(row-1)*M+(col+1)]-row)*(dev_P2[N*M+(row-1)*M+(col+1)]-row);
			if (L[1]<Lmin){
				Lmin = L[1];
				p = 1;
			}
		}
		if(row<(N-1)){
			L[3] = (dev_P2[(row+1)*M+(col+1)]-col)*(dev_P2[(row+1)*M+(col+1)]-col) + (dev_P2[N*M+(row+1)*M+(col+1)]-row)*(dev_P2[N*M+(row+1)*M+(col+1)]-row);
			if (L[3]<Lmin){
				Lmin = L[3];
				p = 3;
			}
		}
		switch (p){
			case 0:
				dev_P[row*M+col] = dev_P2[row*M+col];
				dev_P[N*M+row*M+col] = dev_P2[N*M+row*M+col];
				break;
			case 1:
				dev_P[row*M+col] = dev_P2[(row-1)*M+(col+1)];
				dev_P[N*M+row*M+col] = dev_P2[N*M+(row-1)*M+(col+1)];
				break;
			case 2:
				dev_P[row*M+col] = dev_P2[row*M+col+1];
				dev_P[N*M+row*M+col] = dev_P2[N*M+row*M+col+1];
				break;
			case 3:
				dev_P[row*M+col] = dev_P2[(row+1)*M+(col+1)];
				dev_P[N*M+row*M+col] = dev_P2[N*M+(row+1)*M+(col+1)];
				break;
		}
	}
}
__global__ void propagation2bKernel(int * dev_P, int * dev_P2, int N, int M, int col){
	int row = blockDim.x * blockIdx.x + threadIdx.x;
	//int row = blockDim.x*threadIdx.y + threadIdx.x;
	int L[4];
	int Lmin;
	int p;
	if(row<N){
		col--;
		if(col>=0){
			L[0] = (dev_P[row*M+col]-col)*(dev_P[row*M+col]-col) + (dev_P[N*M+row*M+col]-row)*(dev_P[N*M+row*M+col]-row);
			Lmin = L[0];
			p = 0;
			L[2] = (dev_P[row*M+col+1]-col)*(dev_P[row*M+col+1]-col) + (dev_P[N*M+row*M+col+1]-row)*(dev_P[N*M+row*M+col+1]-row);
			if (L[2]<Lmin){
				Lmin = L[2];
				p = 2;
			}
			if(row>0){
				L[1] = (dev_P[(row-1)*M+(col+1)]-col)*(dev_P[(row-1)*M+(col+1)]-col) + (dev_P[N*M+(row-1)*M+(col+1)]-row)*(dev_P[N*M+(row-1)*M+(col+1)]-row);
				if (L[1]<Lmin){
					Lmin = L[1];
					p = 1;
				}
			}
			if(row<(N-1)){
				L[3] = (dev_P[(row+1)*M+(col+1)]-col)*(dev_P[(row+1)*M+(col+1)]-col) + (dev_P[N*M+(row+1)*M+(col+1)]-row)*(dev_P[N*M+(row+1)*M+(col+1)]-row);
				if (L[3]<Lmin){
					Lmin = L[3];
					p = 3;
				}
			}
			switch (p){
				case 0:
					dev_P2[row*M+col] = dev_P[row*M+col];
					dev_P2[N*M+row*M+col] = dev_P[N*M+row*M+col];
					break;
				case 1:
					dev_P2[row*M+col] = dev_P[(row-1)*M+(col+1)];
					dev_P2[N*M+row*M+col] = dev_P[N*M+(row-1)*M+(col+1)];
					break;
				case 2:
					dev_P2[row*M+col] = dev_P[row*M+col+1];
					dev_P2[N*M+row*M+col] = dev_P[N*M+row*M+col+1];
					break;
				case 3:
					dev_P2[row*M+col] = dev_P[(row+1)*M+(col+1)];
					dev_P2[N*M+row*M+col] = dev_P[N*M+(row+1)*M+(col+1)];
					break;
			}
		}
	}
}

__global__ void propagation3aKernel(int * dev_P, int * dev_P2, int N, int M, int row){
	int col = blockDim.x * blockIdx.x + threadIdx.x;
	//int col = blockDim.x*threadIdx.y + threadIdx.x;
	int L[4];
	int Lmin;
	int p;
	if(col<M){
		L[0] = (dev_P[row*M+col]-col)*(dev_P[row*M+col]-col) + (dev_P[N*M+row*M+col]-row)*(dev_P[N*M+row*M+col]-row);
		Lmin = L[0];
		p = 0;
		L[2] = (dev_P[(row-1)*M+col]-col)*(dev_P[(row-1)*M+col]-col) + (dev_P[N*M+(row-1)*M+col]-row)*(dev_P[N*M+(row-1)*M+col]-row);
		if (L[2]<Lmin){
			Lmin = L[2];
			p = 2;
		}
		if(col>0){
			L[1] = (dev_P[(row-1)*M+(col-1)]-col)*(dev_P[(row-1)*M+(col-1)]-col) + (dev_P[N*M+(row-1)*M+(col-1)]-row)*(dev_P[N*M+(row-1)*M+(col-1)]-row);
			if (L[1]<Lmin){
				Lmin = L[1];
				p = 1;
			}
		}
		if(col<(M-1)){
			L[3] = (dev_P[(row-1)*M+(col+1)]-col)*(dev_P[(row-1)*M+(col+1)]-col) + (dev_P[N*M+(row-1)*M+(col+1)]-row)*(dev_P[N*M+(row-1)*M+(col+1)]-row);
			if (L[3]<Lmin){
				Lmin = L[3];
				p = 3;
			}
		}	
		switch (p){
			case 0:
				dev_P2[row*M+col] = dev_P[row*M+col];
				dev_P2[N*M+row*M+col] = dev_P[N*M+row*M+col];
				break;
			case 1:
				dev_P2[row*M+col] = dev_P[(row-1)*M+(col-1)];
				dev_P2[N*M+row*M+col] = dev_P[N*M+(row-1)*M+(col-1)];
				break;
			case 2:
				dev_P2[row*M+col] = dev_P[(row-1)*M+col];
				dev_P2[N*M+row*M+col] = dev_P[N*M+(row-1)*M+col];
				break;
			case 3:
				dev_P2[row*M+col] = dev_P[(row-1)*M+(col+1)];
				dev_P2[N*M+row*M+col] = dev_P[N*M+(row-1)*M+(col+1)];
				break;
		}
	}
}
__global__ void propagation3bKernel(int * dev_P, int * dev_P2, int N, int M, int row){
	int col = blockDim.x * blockIdx.x + threadIdx.x;
	//int col = blockDim.x*threadIdx.y + threadIdx.x;
	int L[4];
	int Lmin;
	int p;
	if(col<M){
		row++;
		if(row<N){
			L[0] = (dev_P2[row*M+col]-col)*(dev_P2[row*M+col]-col) + (dev_P2[N*M+row*M+col]-row)*(dev_P2[N*M+row*M+col]-row);
			Lmin = L[0];
			p = 0;
			L[2] = (dev_P2[(row-1)*M+col]-col)*(dev_P2[(row-1)*M+col]-col) + (dev_P2[N*M+(row-1)*M+col]-row)*(dev_P2[N*M+(row-1)*M+col]-row);
			if (L[2]<Lmin){
				Lmin = L[2];
				p = 2;
			}
			if(col>0){
				L[1] = (dev_P2[(row-1)*M+(col-1)]-col)*(dev_P2[(row-1)*M+(col-1)]-col) + (dev_P2[N*M+(row-1)*M+(col-1)]-row)*(dev_P2[N*M+(row-1)*M+(col-1)]-row);
				if (L[1]<Lmin){
					Lmin = L[1];
					p = 1;
				}
			}
			if(col<(M-1)){
				L[3] = (dev_P2[(row-1)*M+(col+1)]-col)*(dev_P2[(row-1)*M+(col+1)]-col) + (dev_P2[N*M+(row-1)*M+(col+1)]-row)*(dev_P2[N*M+(row-1)*M+(col+1)]-row);
				if (L[3]<Lmin){
					Lmin = L[3];
					p = 3;
				}
			}
			switch (p){
				case 0:
					dev_P[row*M+col] = dev_P2[row*M+col];
					dev_P[N*M+row*M+col] = dev_P2[N*M+row*M+col];
					break;
				case 1:
					dev_P[row*M+col] = dev_P2[(row-1)*M+(col-1)];
					dev_P[N*M+row*M+col] = dev_P2[N*M+(row-1)*M+(col-1)];
					break;
				case 2:
					dev_P[row*M+col] = dev_P2[(row-1)*M+col];
					dev_P[N*M+row*M+col] = dev_P2[N*M+(row-1)*M+col];
					break;
				case 3:
					dev_P[row*M+col] = dev_P2[(row-1)*M+(col+1)];
					dev_P[N*M+row*M+col] = dev_P2[N*M+(row-1)*M+(col+1)];
					break;
			}
		}
	}
}
	
__global__ void propagation4aKernel(int * dev_P, int * dev_P2, int N, int M, int row){
	int col = blockDim.x * blockIdx.x + threadIdx.x;
	//int col = blockDim.x*threadIdx.y + threadIdx.x;
	int L[4];
	int Lmin;
	int p;
	if(col<M){
		L[0] = (dev_P2[row*M+col]-col)*(dev_P2[row*M+col]-col) + (dev_P2[N*M+row*M+col]-row)*(dev_P2[N*M+row*M+col]-row);
		Lmin = L[0];
		p =0;
		L[2] = (dev_P2[(row+1)*M+col]-col)*(dev_P2[(row+1)*M+col]-col) + (dev_P2[N*M+(row+1)*M+col]-row)*(dev_P2[N*M+(row+1)*M+col]-row);
		if (L[2]<Lmin){
			Lmin = L[2];
			p = 2;
		}
		if(col>0){
			L[1] = (dev_P2[(row+1)*M+col-1]-col)*(dev_P2[(row+1)*M+col-1]-col) + (dev_P2[N*M+(row+1)*M+col-1]-row)*(dev_P2[N*M+(row+1)*M+col-1]-row);
			if (L[1]<Lmin){
				Lmin = L[1];
				p = 1;
			}
		}
		if(col<(M-1)){
			L[3] = (dev_P2[(row+1)*M+(col+1)]-col)*(dev_P2[(row+1)*M+(col+1)]-col) + (dev_P2[N*M+(row+1)*M+(col+1)]-row)*(dev_P2[N*M+(row+1)*M+(col+1)]-row);
			if (L[3]<Lmin){
				Lmin = L[3];
				p = 3;
			}
		}
		switch (p){
			case 0:
				dev_P[row*M+col] = dev_P2[row*M+col];
				dev_P[N*M+row*M+col] = dev_P2[N*M+row*M+col];
				break;
			case 1:
				dev_P[row*M+col] = dev_P2[(row+1)*M+col-1];
				dev_P[N*M+row*M+col] = dev_P2[N*M+(row+1)*M+col-1];
				break;
			case 2:
				dev_P[row*M+col] = dev_P2[(row+1)*M+col];
				dev_P[N*M+row*M+col] = dev_P2[N*M+(row+1)*M+col];
				break;
			case 3:
				dev_P[row*M+col] = dev_P2[(row+1)*M+(col+1)];
				dev_P[N*M+row*M+col] = dev_P2[N*M+(row+1)*M+(col+1)];
				break;
		}
	}
}
__global__ void propagation4bKernel(int * dev_P, int * dev_P2, int N, int M, int row){
	int col = blockDim.x * blockIdx.x + threadIdx.x;
	//int col = blockDim.x*threadIdx.y + threadIdx.x;
	int L[4];
	int Lmin;
	int p;
	if(col<M){
		row--;
		if(row>=0){
			L[0] = (dev_P[row*M+col]-col)*(dev_P[row*M+col]-col) + (dev_P[N*M+row*M+col]-row)*(dev_P[N*M+row*M+col]-row);
			Lmin = L[0];
			p = 0;
			L[2] = (dev_P[(row+1)*M+col]-col)*(dev_P[(row+1)*M+col]-col) + (dev_P[N*M+(row+1)*M+col]-row)*(dev_P[N*M+(row+1)*M+col]-row);
			if (L[2]<Lmin){
				Lmin = L[2];
				p = 2;
			}
			if(col>0){
				L[1] = (dev_P[(row+1)*M+col-1]-col)*(dev_P[(row+1)*M+col-1]-col) + (dev_P[N*M+(row+1)*M+col-1]-row)*(dev_P[N*M+(row+1)*M+col-1]-row);
				if (L[1]<Lmin){
					Lmin = L[1];
					p = 1;
				}
			}
			if(col<(M-1)){
				L[3] = (dev_P[(row+1)*M+(col+1)]-col)*(dev_P[(row+1)*M+(col+1)]-col) + (dev_P[N*M+(row+1)*M+(col+1)]-row)*(dev_P[N*M+(row+1)*M+(col+1)]-row);
				if (L[3]<Lmin){
					Lmin = L[3];
					p = 3;
				}
			}
			switch (p){
				case 0:
					dev_P2[row*M+col] = dev_P[row*M+col];
					dev_P2[N*M+row*M+col] = dev_P[N*M+row*M+col];
					break;
				case 1:
					dev_P2[row*M+col] = dev_P[(row+1)*M+col-1];
					dev_P2[N*M+row*M+col] = dev_P[N*M+(row+1)*M+col-1];
					break;
				case 2:
					dev_P2[row*M+col] = dev_P[(row+1)*M+col];
					dev_P2[N*M+row*M+col] = dev_P[N*M+(row+1)*M+col];
					break;
				case 3:
					dev_P2[row*M+col] = dev_P[(row+1)*M+(col+1)];
					dev_P2[N*M+row*M+col] = dev_P[N*M+(row+1)*M+(col+1)];
					break;
			}
		}
	}
	__syncthreads();
}

//cudaError_t transform( int * S, float * D, int N, int M);

cudaError_t transform(int * S, float * D, int N, int M){
	cudaError_t cudaStatus;

	cudaEvent_t start, stop;
	cudaEventCreate(&start);
	cudaEventCreate(&stop);

	// Choose which GPU to run on, change this on a multi-GPU system.
    cudaStatus = cudaSetDevice(0);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
        goto Error;
    }

    // Allocate GPU buffers for vectors
    cudaStatus = cudaMalloc((void**)&dev_S, (N*M) * sizeof(int));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc failed!");
        goto Error;
    }

    cudaStatus = cudaMalloc((void**)&dev_P, 2*N*M*sizeof(int));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc failed!");
        goto Error;
    }

	cudaStatus = cudaMalloc((void**)&dev_P2, 2*N*M*sizeof(int));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc failed!");
        goto Error;
    }

    cudaStatus = cudaMalloc((void**)&dev_D, N*M*sizeof(float));
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMalloc failed!");
        goto Error;
    }

    // Copy input vectors from host memory to GPU buffers.
    cudaStatus = cudaMemcpy(dev_S, S, N*M*sizeof(int), cudaMemcpyHostToDevice);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy HtD failed!");
        goto Error;
    }

	//dim3 dimBlock1(256, ceil(N/256.0), 1);
	//dim3 dimBlock2(256, ceil(M/256.0), 1);
	//dim3 dimGrid(1, 1, 1);
	dim3 dimBlock(1024, 1, 1);
	dim3 dimGrid1(ceil(N/1024.0));
	dim3 dimGrid2(ceil(M/1024.0));
	dim3 dimBlock3(ceil(M/256.0), ceil(N/256.0), 1);
	dim3 dimGrid3(256, 256, 1);

	cudaEventRecord(start, 0);

	initializationKernel<<<dimGrid3, dimBlock3>>>(dev_S, dev_P, dev_P2, N, M);
	
	int i;
	for(i=1; i<M;i+=2){
		propagation1aKernel<<<dimGrid1, dimBlock>>>(dev_P, dev_P2, N, M, i);
		propagation1bKernel<<<dimGrid1, dimBlock>>>(dev_P, dev_P2, N, M, i);
	}
	if(M%2==1){
		copy1ColKernel<<<dimGrid1, dimBlock>>>(dev_P, dev_P2, N, M, M-1);
		for(i=M-2; i>=0;i-=2){
			propagation2aKernel<<<dimGrid1, dimBlock>>>(dev_P, dev_P2, N, M, i);
			propagation2bKernel<<<dimGrid1, dimBlock>>>(dev_P, dev_P2, N, M, i);
		}
	} else{
		copy1ColKernel<<<dimGrid1, dimBlock>>>(dev_P2, dev_P, N, M, M-1);
		for(i=M-2; i>=0;i-=2){
			propagation2aKernel<<<dimGrid1, dimBlock>>>(dev_P2, dev_P, N, M, i);
			propagation2bKernel<<<dimGrid1, dimBlock>>>(dev_P2, dev_P, N, M, i);
		}
	}
	copyColKernel<<<dimGrid3, dimBlock3>>>(dev_P, dev_P2, N, M);
	
	int j;
	for(j=1; j<N;j+=2){
		propagation3aKernel<<<dimGrid2, dimBlock>>>(dev_P, dev_P2, N, M, j);
		propagation3bKernel<<<dimGrid2, dimBlock>>>(dev_P, dev_P2, N, M, j);
	}
	if(N%2==1){
		copy1RowKernel<<<dimGrid2, dimBlock>>>(dev_P, dev_P2, N, M, N-1);
		for(j=N-2; j>=0;j-=2){
			propagation4aKernel<<<dimGrid2, dimBlock>>>(dev_P, dev_P2, N, M, j);
			propagation4bKernel<<<dimGrid2, dimBlock>>>(dev_P, dev_P2, N, M, j);
		}
	} else{
		copy1RowKernel<<<dimGrid2, dimBlock>>>(dev_P2, dev_P, N, M, N-1);
		for(j=N-2; j>=0; j-=2){
			propagation4aKernel<<<dimGrid2, dimBlock>>>(dev_P2, dev_P, N, M, j);
			propagation4bKernel<<<dimGrid2, dimBlock>>>(dev_P2, dev_P, N, M, j);
		}
	}
	copyRowKernel<<<dimGrid3, dimBlock3>>>(dev_P, dev_P2, N, M);
	
	distanceKernel<<<dimGrid3, dimBlock3>>>(dev_D, dev_P, N, M);
	
	cudaStatus = cudaDeviceSynchronize();
	cudaEventRecord(stop, 0); 
	cudaEventSynchronize(stop); 

    // Check for any errors launching the kernel
    cudaStatus = cudaGetLastError();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "Kernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
        goto Error;
    }
    
    // cudaDeviceSynchronize waits for the kernel to finish, and returns
    // any errors encountered during the launch.
    cudaStatus = cudaDeviceSynchronize();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaDeviceSynchronize returned error code %d after launching Kernel!\n", cudaStatus);
        goto Error;
    }

	cudaStatus = cudaMemcpy(D, dev_D, N*M*sizeof(float), cudaMemcpyDeviceToHost);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaMemcpy DtH failed!");
        goto Error;
    }

	cudaEventSynchronize(stop);
	float milliseconds = 0;
	cudaEventElapsedTime(&milliseconds, start, stop);
//    printf("Execution Time: %f ms\n", milliseconds);
	cudaEventDestroy(start);
	cudaEventDestroy(stop);

Error:
    cudaFree(dev_S);
    cudaFree(dev_P);
	cudaFree(dev_P2);
	cudaFree(dev_D);
	return cudaStatus;
}
