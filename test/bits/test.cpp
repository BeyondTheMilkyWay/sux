#include <gtest/gtest.h>

#include "../xoroshiro128pp.hpp"
#include "rankselect.hpp"
#include "dynranksel.hpp"

#define LEAF 4
#define NKEYS_TEST 1000000
#include "recsplit.hpp"

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
