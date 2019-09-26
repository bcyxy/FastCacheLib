
test_run: example.cpp f_cache.hpp
	g++ -g -o test_run example.cpp -lpthread

.PHONY: clean
clean:
	rm -rf *.o test_run
