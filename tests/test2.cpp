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

    unsigned char buf[0xff];

    obitstream obs = obitstream::ref(buf);
    ibitstream ibs = ibitstream::ref(buf);
    obs.write_bit(1);
    obs.write_bit(0);
    obs.write_intS(1, 5);
    obs.write_intS(-1, 5);
    obs.write_int(2, 5);
    obs.write_int(-2, 5);
    obs.write_uint(3, 5);
    obs.write_uint(-3, 5);
    TEST_ASSERT(ibs.read_bit() == 1);
    TEST_ASSERT(ibs.read_bit() == 0);
    TEST_ASSERT(ibs.read_intS(5) == 1);
    TEST_ASSERT(ibs.read_intS(5) == -1);
    TEST_ASSERT(ibs.read_int(5) == 2);
    TEST_ASSERT(ibs.read_int(5) == -2);
    TEST_ASSERT(ibs.read_int(5) == 3);
    TEST_ASSERT(ibs.read_uint(5) == (-3 & 0x1f));
    cout << ibs << endl;

    return EXIT_SUCCESS;
}
