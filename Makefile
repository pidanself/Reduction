NVCC_OPTS=-O3 -arch=sm_80

SRCs = main.cpp v0_naive.cpp
OBJs = main.o v0_naive.o

reduce: $(OBJs)
	nvcc -o main $(OBJs) $(NVCC_OPTS)

%.o: %.cpp
	nvcc -c $^ -o $@ $(NVCC_OPTS)

clean:
	rm -f *.o main