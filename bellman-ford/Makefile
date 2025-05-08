cpp:
	g++ -O3 -o main-cpp.o cpp.cpp

dpc:
	icpx -fsycl -fsycl-targets=nvptx64-nvidia-cuda -o main-dpc.o dpc.cpp

openmp:
	clang++ -fopenmp -fopenmp-targets=nvptx64-nvidia-cuda -Xopenmp-target -march=sm_75 -O3 -o main-openmp.o openmp.cpp

all:
	make cpp
	make dpc
	make openmp

clean:
	rm -f main-cpp.o main-dpc.o main-openmp.o