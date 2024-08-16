CC=$(shell brew --prefix llvm)/bin/clang++ -L$(shell brew --prefix)/lib -std=c++17 -pthread -fopenmp

debug:
	$(CC) main.cpp -o threadpool_benchmark_debug -ggdb
	./threadpool_benchmark_debug 

threadpool_benchmark:
	$(CC) main.cpp -o threadpool_benchmark -O3
	./threadpool_benchmark

.DEFAULT_GOAL := threadpool_benchmark
.PHONY: debug threadpool_benchmark

