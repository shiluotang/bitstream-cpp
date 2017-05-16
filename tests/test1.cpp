#include "../src/bitstream.hpp"

#include <cstdlib>
#include <iostream>

#define TEST_ASSERT(CONDITION) \
    do { \
        if (!(CONDITION)) { \
            std::cerr << #CONDITION << " failed!" << std::endl; \
            return EXIT_FAILURE; \
        } \
    } while (0)

int main(int argc, char* argv[]) {
    using namespace std;
    using namespace org::sqg;

    uint16_t x = 1;

    ibitstream ibs = ibitstream::ref(&x, sizeof(x));

    if (*reinterpret_cast<uint8_t*>(&x) == 0) {
        TEST_ASSERT(ibs.read_int(8) == 0);
        TEST_ASSERT(ibs.read_int(8) == 1);
    } else {
        TEST_ASSERT(ibs.read_int(8) == 1);
        TEST_ASSERT(ibs.read_int(8) == 0);
    }
    cout << ibs << endl;

    return EXIT_SUCCESS;
}
