#include "../sphash.h"
#include <random>

int main(void)
{
	std::random_device device;
	std::mt19937 rand(device());
	uint8_t data[64*1024];
	for(uint32_t i=0; i<(64*1024); ++i){
		data[i] = static_cast<uint8_t>(rand()&0xFFU);
	}
	std::uniform_int<uint32_t> dist(0, 64*1024-1);
	for(uint32_t i=0; i<100; ++i){
		size_t size = dist(rand);
		sph::sphash64(size, data);
	}
	return 0;
}
