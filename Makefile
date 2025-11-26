.PHONY: config res build run test

config:
	cmake -S . -B ./build -DCMAKE_SYSTEM_NAME=Linux -G "Ninja Multi-Config"

# res:
# ifneq (,$(wildcard ./build/resources))
# 	rm ./build/resources
# endif
# ifneq (,$(wildcard ./assets/resources))
# 	rm ./assets/resources
# endif
# 	cmake --build build --config Release --target resource_packer
# 	./build/tools/resource_packer/Release/resource_packer ./assets ./assets/resources

build:
	cmake --build build --config Debug --target ufps

run: build
	DRI_PRIME=1 ./build/src/Debug/ufps ./assets

clean:
	cmake --build build --target clean

test:
	cmake --build build --config Debug --target unit_tests
	ctest --test-dir ./build -C Debug --progress -j

