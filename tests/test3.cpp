#include "../src/bitstream.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <ctime>
#include <stdexcept>

#define TEST_ASSERT(CONDITION) \
    do { \
        if (!(CONDITION)) { \
            std::cerr << #CONDITION << " failed!" << std::endl; \
            return EXIT_FAILURE; \
        } \
    } while (0)

typedef int perf_runner(void*);

static int perf_test(perf_runner *runner, std::string const& name, std::size_t N, void* param) try {
    using namespace std;
    time_t t1, t2;
    t1 = time(NULL);
    for (size_t i = 0; i < N; ++i)
        if (runner(param) == EXIT_FAILURE)
            return EXIT_FAILURE;
    t2 = time(NULL);
    cout << "Name = " << name
        << ", N = " << N
        << ", Total = " << difftime(t2, t1) << " s"
        << ", Average = " << difftime(t2, t1) / N << " s"
        << ", QPS = " << N / difftime(t2, t1)
        << endl;
    return EXIT_SUCCESS;
} catch (std::exception const &e) {
    std::cerr << "[Exception]: " << e.what() << std::endl;
    return EXIT_FAILURE;
} catch (...) {
    std::cerr << "[Exception]: UNKNOWN" << std::endl;
    return EXIT_FAILURE;
}

#define PERF_TEST(FUNCTION, N, PARAM) perf_test(&FUNCTION, #FUNCTION, N, PARAM)

template <typename BitStream>
struct position_guard {
    position_guard(BitStream &stream)
        :_M_stream_ref(stream), _M_pos(stream.tell()) {
    }
    ~position_guard() {
        _M_stream_ref.seek(_M_pos);
    }

    BitStream &_M_stream_ref;
    size_t _M_pos;
};

static int test_output_perf(void *param) {
    using namespace std;
    using namespace org::sqg;
    obitstream &obs = *static_cast<obitstream*>(param);
    try {
        position_guard<obitstream> guard(obs);
        obs.write_bit(1);
        obs.write_bit(0);
        obs.write_intS(1, 5);
        obs.write_intS(-1, 5);
        obs.write_int(2, 5);
        obs.write_int(-2, 5);
        obs.write_uint(3, 5);
        obs.write_uint(-3, 5);
    } catch (bitstream_error const &e) {
        return EXIT_FAILURE;
    } catch (std::exception const &e) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int test_input_perf(void *param) {
    using namespace std;
    using namespace org::sqg;
    ibitstream &ibs = *static_cast<ibitstream*>(param);
    try {
        position_guard<ibitstream> guard(ibs);
        TEST_ASSERT(ibs.read_bit() == 1);
        TEST_ASSERT(ibs.read_bit() == 0);
        TEST_ASSERT(ibs.read_intS(5) == 1);
        TEST_ASSERT(ibs.read_intS(5) == -1);
        TEST_ASSERT(ibs.read_int(5) == 2);
        TEST_ASSERT(ibs.read_int(5) == -2);
        TEST_ASSERT(ibs.read_int(5) == 3);
        TEST_ASSERT(ibs.read_uint(5) == (-3 & 0x1f));
    } catch (bitstream_error const &e) {
        return EXIT_FAILURE;
    } catch (std::exception const &e) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    using namespace std;
    using namespace org::sqg;

    unsigned char buf[0xff];
    size_t const N = 0x1ffffff;

    obitstream obs = obitstream::ref(buf);
    ibitstream ibs = ibitstream::ref(buf);

    try {
        if (PERF_TEST(test_output_perf, N, &obs) == EXIT_FAILURE)
            return EXIT_FAILURE;
        if (PERF_TEST(test_input_perf, N, &ibs) == EXIT_FAILURE)
            return EXIT_FAILURE;
    } catch (...) {
        cerr << "[Exception]: UNKNOWN" << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
