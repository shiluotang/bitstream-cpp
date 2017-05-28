#include "bitstream.hpp"

#include <streambuf>
#include <ostream>
#include <vector>

struct org::sqg::obitstream::impl {

    virtual void seek(std::streamoff, std::ios::seekdir dir) = 0;

    virtual std::streampos tell() const = 0;

    virtual void* data() = 0;

    virtual void const* data() const = 0;

    virtual std::size_t size() const = 0;

    virtual bool ensure_more_space(size_t) = 0;

    virtual void write_bit0(size_t) = 0;

    virtual void write_uint0(uint64_t, size_t) = 0;

    void write_bit(size_t bit) {
        if (!ensure_more_space(1))
            throw bitstream_error();
        write_bit0(bit);
    };

    virtual void write_int(int64_t value, size_t bits) {
        if (!ensure_more_space(bits))
            throw bitstream_error();
        write_uint0(value, bits);
    }

    virtual void write_uint(uint64_t value, size_t bits) {
        if (!ensure_more_space(bits))
            throw bitstream_error();
        write_uint0(value, bits);
    }

    virtual void write_intS(int64_t value, size_t bits) {
        if (!ensure_more_space(bits))
            throw bitstream_error();
        if (value < 0) {
            write_bit0(1);
            write_uint0(-value, bits - 1);
        } else {
            write_bit0(0);
            write_uint0(value, bits - 1);
        }
    }
};

namespace {

    uint8_t const BITS_MASKS[] = {
        0,
        0x1,
        (0x1 << 1) | 0x1,
        (0x1 << 2) | (0x1 << 1) | 0x1,
        (0x1 << 3) | (0x1 << 2) | (0x1 << 1) | 0x1,
        (0x1 << 4) | (0x1 << 3) | (0x1 << 2) | (0x1 << 1) | 0x1,
        (0x1 << 5) | (0x1 << 4) | (0x1 << 3) | (0x1 << 2) | (0x1 << 1) | 0x1,
        (0x1 << 6) | (0x1 << 5) | (0x1 << 4) | (0x1 << 3) | (0x1 << 2) | (0x1 << 1) | 0x1,
        (0x1 << 7) | (0x1 << 6) | (0x1 << 5) | (0x1 << 4) | (0x1 << 3) | (0x1 << 2) | (0x1 << 1) | 0x1,
    };

    uint8_t const SET_BIT_MASKS[] = {
        0x1 << 7, 0x1 << 6, 0x1 << 5, 0x1 << 4,
        0x1 << 3, 0x1 << 2, 0x1 << 1, 0x1 << 0,
    };

    uint8_t const CLEAR_BIT_MASKS[] = {
        (~(0x1 << 7)) & 0xff, (~(0x1 << 6)) & 0xff, (~(0x1 << 5)) & 0xff, (~(0x1 << 4)) & 0xff,
        (~(0x1 << 3)) & 0xff, (~(0x1 << 2)) & 0xff, (~(0x1 << 1)) & 0xff, (~(0x1 << 0)) & 0xff,
    };

    struct no_delete {
        void operator () (void *p) { }
        void operator () (void const *p) { }
    };

    struct fixed :public org::sqg::obitstream::impl {
        fixed(std::shared_ptr<org::sqg::byte> const &ptr, std::size_t size)
            :_M_sptr(ptr),
            _M_bytes(ptr.get()),
            _M_pos(0),
            _M_size(size)
        {
        }

        virtual ~fixed() {
            _M_sptr.reset();
            _M_bytes = _M_sptr.get();
            _M_pos = 0;
            _M_size = 0;
        }

        virtual void seek(std::streamoff offset, std::ios::seekdir dir) {
            using namespace std;
            switch(dir) {
                case ios::beg:
                    _M_pos = 0 + offset;
                    break;
                case ios::cur:
                    _M_pos = _M_pos + offset;
                    break;
                case ios::end:
                    _M_pos = _M_size + offset;
                    break;
                default: break;
            }
        }

        virtual std::streampos tell() const { return _M_pos; }

        virtual void* data() { return _M_bytes; }

        virtual void const* data() const { return _M_bytes; }

        virtual size_t size() const { return _M_size; }

        virtual bool ensure_more_space(size_t n) {
            return _M_pos + n <= _M_size;
        }

        virtual void write_bit0(size_t bit) {
            if (bit & 0x1) {
                _M_bytes[_M_pos >> 3] |= SET_BIT_MASKS[_M_pos & 0x7];
            } else {
                _M_bytes[_M_pos >> 3] &= CLEAR_BIT_MASKS[_M_pos & 0x7];
            }
            ++_M_pos;

        }

        virtual void write_uint0(uint64_t value, size_t bits) {
            uint8_t preserve = 0;
            size_t p = 0;
            while (bits > 0) {
                if ((_M_pos & 0x7) == 0) {
                    while (bits >= 8) {
                        bits -= 8;
                        _M_bytes[_M_pos >> 3] = (value >> bits) & 0xff;
                        _M_pos += 8;
                    }
                    if (bits > 0) {
                        p = _M_pos >> 3;
                        preserve = _M_bytes[p] & BITS_MASKS[8 - bits];
                        _M_bytes[p] = value << (8 - bits);
                        _M_bytes[p] |= preserve;
                        _M_pos += bits;
                        bits = 0;
                        break;
                    }
                } else {
                    p = _M_pos >> 3;
                    size_t first_bit_of_next_byte_pos = (p + 1) << 3;
                    if (_M_pos + bits >= first_bit_of_next_byte_pos) {
                        size_t rbits = first_bit_of_next_byte_pos - _M_pos;
                        // just cross the byte boundary or on it.
                        preserve = (_M_bytes[p] & (~BITS_MASKS[rbits])) & 0xff;
                        _M_bytes[p] = (value >> (bits - rbits)) & BITS_MASKS[rbits];
                        _M_bytes[p] |= preserve;
                        _M_pos += rbits;
                        bits -= rbits;
                    } else {
                        size_t rbits = first_bit_of_next_byte_pos - (_M_pos + bits);
                        preserve = _M_bytes[p] & (~(BITS_MASKS[bits] << rbits)) & 0xff;
                        _M_bytes[p] = (value & BITS_MASKS[bits]) << rbits;
                        _M_bytes[p] |= preserve;
                        _M_pos += bits;
                        bits = 0;
                        break;
                    }
                }
            }
        }

        std::shared_ptr<org::sqg::byte> _M_sptr;
        org::sqg::byte  *_M_bytes;
        std::size_t     _M_pos;
        std::size_t     _M_size;
    };

    struct unfixed :public org::sqg::obitstream::impl {
        unfixed()
            :_M_data(16),
            _M_pos(0),
            _M_size(16 * 8)
        {
        }

        virtual ~unfixed() {
            std::vector<org::sqg::byte>().swap(_M_data);
            _M_pos = 0;
            _M_size = _M_data.size() * 8;
        }

        virtual void seek(std::streamoff offset, std::ios::seekdir dir) {
            using namespace std;
            switch(dir) {
                case ios::beg:
                    _M_pos = 0 + offset;
                    break;
                case ios::cur:
                    _M_pos = _M_pos + offset;
                    break;
                case ios::end:
                    _M_pos = _M_size + offset;
                    break;
                default: break;
            }
        }

        virtual std::streampos tell() const { return _M_pos; }

        virtual void* data() { return &_M_data[0]; }

        virtual void const* data() const { return &_M_data[0]; }

        virtual std::size_t size() const { return _M_size; }

        virtual bool ensure_more_space(size_t n) {
            if (_M_pos + n > _M_size) {
                _M_data.resize(_M_size >> 2);
                _M_size <<= 1;
            }
            return true;
        }

        virtual void write_bit0(size_t bit) {
            if (bit & 0x1) {
                _M_data[_M_pos >> 3] |= SET_BIT_MASKS[_M_pos & 0x7];
            } else {
                _M_data[_M_pos >> 3] &= CLEAR_BIT_MASKS[_M_pos & 0x7];
            }
            ++_M_pos;
        }

        virtual void write_uint0(uint64_t value, size_t bits) {
            uint8_t preserve = 0;
            size_t p = 0;
            while (bits > 0) {
                if ((_M_pos & 0x7) == 0) {
                    while (bits >= 8) {
                        bits -= 8;
                        _M_data[_M_pos >> 3] = (value >> bits) & 0xff;
                        _M_pos += 8;
                    }
                    if (bits > 0) {
                        p = _M_pos >> 3;
                        preserve = _M_data[p] & BITS_MASKS[8 - bits];
                        _M_data[p] = value & BITS_MASKS[bits];
                        _M_data[p] |= preserve;
                        _M_pos += bits;
                        bits = 0;
                    }
                    break;
                } else {
                    p = _M_pos >> 3;
                    size_t first_bit_of_next_byte_pos = (p + 1) << 3;
                    if (_M_pos + bits >= first_bit_of_next_byte_pos) {
                        size_t rbits = first_bit_of_next_byte_pos - _M_pos;
                        preserve = _M_data[p] & (~BITS_MASKS[rbits]) & 0xff;
                        _M_data[p] = value >> (bits - rbits);
                        _M_data[p] |= preserve;
                        _M_pos += rbits;
                        bits -= rbits;
                    } else {
                        size_t rbits = first_bit_of_next_byte_pos - (_M_pos + bits);
                        preserve = _M_data[p] & (~(BITS_MASKS[bits] << rbits)) & 0xff;
                        _M_data[p] = (value & BITS_MASKS[bits]) << rbits;
                        _M_data[p] |= preserve;
                        _M_pos += bits;
                        bits = 0;
                    }
                    break;
                }
            }
        }

        std::vector<org::sqg::byte> _M_data;
        size_t _M_pos;
        size_t _M_size;
    };
}

namespace org {
    namespace sqg {

        obitstream::obitstream(
                std::shared_ptr<byte> const &mem,
                std::size_t size)
        {
            if (!mem) {
                _M_data = std::make_shared<unfixed>();
            } else {
                _M_data = std::make_shared<fixed>(mem, size * 8);
            }
        }

        void* obitstream::data() {
            return _M_data->data();
        }

        void const* obitstream::data() const {
            return _M_data->data();
        }

        std::size_t obitstream::size() const {
            return _M_data->size();
        }

        obitstream& obitstream::write_bit(std::size_t bit) {
            _M_data->write_bit(bit);
            return *this;
        }

        obitstream& obitstream::write_uint(std::uint64_t value, std::size_t bits) {
            _M_data->write_uint(value, bits);
            return *this;
        }

        obitstream& obitstream::write_intS(std::int64_t value, std::size_t bits) {
            _M_data->write_intS(value, bits);
            return *this;
        }

        obitstream& obitstream::write_int(std::int64_t value, std::size_t bits) {
            _M_data->write_int(value, bits);
            return *this;
        }

        obitstream obitstream::ref(void *mem, std::size_t size) {
            return obitstream(
                    std::shared_ptr<org::sqg::byte>(static_cast<org::sqg::byte*>(mem), no_delete()),
                    size);
        }

        obitstream& obitstream::seek(std::streamoff offset, std::ios::seekdir dir) {
            _M_data->seek(offset, dir);
            return *this;
        }

        std::streampos obitstream::tell() const {
            return _M_data->tell();
        }

        ibitstream::ibitstream(
                std::shared_ptr<byte const> const &mem,
                std::size_t size)
            :_M_sptr(mem),
            _M_bytes(mem.get()),
            _M_size(size),
            _M_pos(0)
        {
        }

        ibitstream::~ibitstream() {
            _M_sptr.reset();
            _M_bytes = NULL;
            _M_size = 0;
        }

        size_t ibitstream::read_bit() {
            size_t bit = 0;
            if (_M_pos + 1 > _M_size)
                throw bitstream_error();
            bit = (_M_bytes[_M_pos >> 3] >> ((~_M_pos) & 0x7)) & 0x1;
            ++_M_pos;
            return bit;
        }

        std::uint64_t ibitstream::read_uint(std::size_t bits) {
            std::uint64_t r = 0;
            if (_M_pos + bits > _M_size)
                throw bitstream_error();
            while (bits > 0) {
                if (_M_pos % 8 == 0) {
                    while (bits >= 8) {
                        r <<= 8;
                        r |= _M_bytes[_M_pos >> 3];
                        _M_pos += 8;
                        bits -= 8;
                    }
                    if (bits > 0) {
                        r <<= bits;
                        r |= (_M_bytes[_M_pos >> 3] >> (8 - bits)) & BITS_MASKS[bits];
                        _M_pos += bits;
                        bits = 0;
                    }
                    break;
                } else {
                    size_t first_bit_of_next_byte_pos = ((_M_pos >> 3) + 1) << 3;
                    if(_M_pos + bits >= first_bit_of_next_byte_pos) {
                        // at least to read the left bits in the low order (LSB).
                        size_t rbits = first_bit_of_next_byte_pos - _M_pos;
                        r <<= rbits;
                        r |= _M_bytes[_M_pos >> 3] & BITS_MASKS[rbits];
                        _M_pos += rbits;
                        bits -= rbits;
                    } else {
                        r <<= bits;
                        r |= (_M_bytes[_M_pos >> 3] >> (first_bit_of_next_byte_pos - (_M_pos + bits))) & BITS_MASKS[bits];
                        _M_pos += bits;
                        bits = 0;
                        break;
                    }
                }
            }
            return r;
        }

        std::int64_t ibitstream::read_int(std::size_t bits) {
            std::int64_t r = 0;
            if (read_bit()) {
                // negative
                r = -1;
                r <<= (bits - 1);
            }
            r |= read_uint(bits - 1);
            return r;
        }

        std::int64_t ibitstream::read_intS(std::size_t bits) {
            std::int64_t r = 0;
            if (read_bit()) {
                r = -read_uint(bits - 1);
            } else {
                r |= read_uint(bits - 1);
            }
            return r;
        }

        std::ostream& operator << (std::ostream &os, ibitstream &ibs) {
            struct pos_guard {
                explicit pos_guard(ibitstream &rhs) :_M_ref(rhs), _M_pos(rhs.tell()) { }
                ~pos_guard() { _M_ref.seek(_M_pos); }

                ibitstream &_M_ref;
                size_t _M_pos;
            } guard(ibs);
            try {
                while (true)
                    os << ibs.read_bit();
            } catch (bitstream_error const &ignore) {
            }
            return os;
        }

        ibitstream ibitstream::ref(void const* mem, std::size_t size) {
            return ibitstream(
                    std::shared_ptr<byte const>(static_cast<byte const*>(mem), no_delete()),
                    size * 8);
        }

        ibitstream& ibitstream::seek(std::streamoff offset, std::ios::seekdir dir) {
            using namespace std;
            switch(dir) {
                case ios::beg:
                    _M_pos = 0 + offset;
                    break;
                case ios::cur:
                    _M_pos = _M_pos + offset;
                    break;
                case ios::end:
                    _M_pos = _M_size + offset;
                    break;
                default: break;
            }
            return *this;
        }
    }
}
