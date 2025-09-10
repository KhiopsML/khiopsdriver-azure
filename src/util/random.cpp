#include "random.hpp"
#include <chrono>
#include <random>

using namespace std;

namespace az
{
	bool RandomBool()
	{
		static random_device randomDevice;
		static minstd_rand::result_type seed =
			randomDevice() ^ (
				(minstd_rand::result_type)chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count()
				+
				(minstd_rand::result_type)chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count()
			);
		static minstd_rand generator(seed);
		return (bool)(generator() % 2 == 1);
	}
}
