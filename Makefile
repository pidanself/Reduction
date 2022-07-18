
NVCC_OPTS=-O3 -arch=sm_80

reduce: main.cu
	nvcc -o main main.cu $(NVCC_OPTS)

clean:
	rm -f main