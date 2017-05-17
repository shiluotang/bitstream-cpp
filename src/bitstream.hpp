#ifndef BITSTREAM_BITSTREAM_HPP_INCLUDED
#define BITSTREAM_BITSTREAM_HPP_INCLUDED

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <ios>
#include <iosfwd>

namespace org {
    namespace sqg {

        typedef uint8_t byte;

        class bitstream_error : public std::runtime_error {
            public:
                bitstream_error(): runtime_error("insufficient memory for bitstream I/O") { }
        };

        class obitstream {
            public:
                obitstream(std::shared_ptr<byte> const&, std::size_t);

                virtual ~obitstream() { _M_data.reset(); }
            public:
                obitstream& seek(std::streamoff, std::ios::seekdir = std::ios::beg);
                std::streampos tell() const;

                obitstream& write_bit (std::size_t);
                obitstream& write_uint(std::uint64_t, std::size_t);
                obitstream& write_intS(std::int64_t,  std::size_t);
                obitstream& write_int (std::int64_t,  std::size_t);

                void*       data();
                void const* data() const;
                std::size_t size() const;
            public:
                static obitstream ref(void*, std::size_t);
                template < typename T, size_t N >
                static obitstream ref(T (&a)[N]) { return ref(&a[0], N * sizeof(a)); }
                template < typename D = std::default_delete<org::sqg::byte[]> >
                static obitstream own(void *mem, std::size_t size, D deleter = D()) {
                    return obitstream(
                            std::shared_ptr<byte>(static_cast<byte*>(mem), deleter),
                            size * 8);
                }
            public:
                struct impl;
            private:
                std::shared_ptr<impl> _M_data;
        };

        class ibitstream {
            public:
                ibitstream(std::shared_ptr<byte const> const&, std::size_t);
                virtual ~ibitstream();
            public:
                std::uint64_t   read_uint(std::size_t);
                std::int64_t    read_int (std::size_t);
                std::int64_t    read_intS(std::size_t);
                std::size_t     read_bit();
            public:
                ibitstream& seek(std::streamoff, std::ios::seekdir = std::ios::beg);
                std::streampos tell() const { return _M_pos; }
            public:
                static ibitstream ref(void const*, std::size_t);
                template <typename T, std::size_t N>
                static ibitstream ref(T (&a)[N]) {
                    return ref(&a[0], sizeof(a));
                }
                template < typename D = std::default_delete<byte[]> >
                static ibitstream own(void const* mem, std::size_t size, D deleter = D()) {
                    return ibitstream(
                            std::shared_ptr<byte const>(static_cast<byte const*>(mem), deleter),
                            size * 8);
                }
            private:
                std::shared_ptr<byte const> _M_sptr;
                byte const  *_M_bytes;
                std::size_t _M_size;
                std::size_t _M_pos;

                friend std::ostream& operator << (std::ostream&, ibitstream&);
        };

        extern std::ostream& operator << (std::ostream&, ibitstream&);
    }
}

#endif // BITSTREAM_BITSTREAM_HPP_INCLUDED
