cpp:
	g++ -O3 -o main-cpp.o cpp.cpp

dpc-cpu:
	icpx -fsycl -fsycl-targets=spir64 -o main-dpc-cpu.o dpc.cpp -DDPC_CPU

dpc-gpu:
	icpx -fsycl -fsycl-targets=nvptx64-nvidia-cuda -o main-dpc-gpu.o dpc.cpp -DDPC_GPU

openmp-cpu:
	clang++ -fopenmp -O3 -o main-openmp-cpu.o openmp.cpp -DOPENMP_CPU

openmp-gpu:
	clang++ -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target -march=sm_75 -O3 -o main-openmp-gpu.o openmp.cpp -DOPENMP_GPU

all:
	make cpp
	make dpc-cpu
	make dpc-gpu
	make openmp-cpu
	make openmp-gpu

clean:
	rm -f main-cpp.o main-dpc-cpu.o main-dpc-gpu.o main-openmp-cpu.o main-openmp-gpu.o