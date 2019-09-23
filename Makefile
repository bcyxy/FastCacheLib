
test_run: example.cpp f_cache.hpp hash_bucket.hpp
	g++ -g -o test_run example.cpp

.PHONY: clean
clean:
	rm -rf *.o test_run
