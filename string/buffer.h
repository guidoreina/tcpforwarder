#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

namespace string {
  class buffer {
    public:
      // Constructor.
      buffer() = default;

      // Move constructor.
      buffer(buffer&& other);

      // Destructor.
      ~buffer();

      // Move assignment operator.
      buffer& operator=(buffer&& other);

      // Swap content.
      void swap(buffer& other);

      // Clear buffer.
      void clear();

      // Get data.
      const void* data() const;

      // Get length.
      size_t length() const;

      // Empty?
      bool empty() const;

      // Get size of allocated storage.
      size_t capacity() const;

      // Get remaining space available.
      size_t remaining() const;

      // Reserve memory.
      bool reserve(size_t n);

      // Resize.
      bool resize(size_t n);

      // Append.
      bool append(const buffer& buf);
      bool append(const buffer& buf, size_t pos, size_t n);
      bool append(const void* buf, size_t n);
      bool append(size_t n, uint8_t c);

      // Append a single character.
      bool push_back(uint8_t c);

      // Insert.
      bool insert(size_t pos, const buffer& buf);
      bool insert(size_t pos1, const buffer& buf, size_t pos2, size_t n);
      bool insert(size_t pos, const void* buf, size_t n);
      bool insert(size_t pos, size_t n, uint8_t c);

      // Erase.
      bool erase(size_t pos = 0, size_t n = ULONG_MAX);

      // Replace.
      bool replace(size_t pos, size_t n, const buffer& buf);
      bool replace(size_t pos1,
                   size_t n1,
                   const buffer& buf,
                   size_t pos2,
                   size_t n2);

      bool replace(size_t pos, size_t n1, const void* buf, size_t n2);
      bool replace(size_t pos, size_t n1, size_t n2, uint8_t c);

    private:
      static constexpr const size_t initial_size = 32;

      uint8_t* _M_data = nullptr;
      size_t _M_size = 0;
      size_t _M_used = 0;

      // Disable copy constructor and assignment operator.
      buffer(const buffer&) = delete;
      buffer& operator=(const buffer&) = delete;
  };

  inline buffer::buffer(buffer&& other)
    : _M_data(other._M_data),
      _M_size(other._M_size),
      _M_used(other._M_used)
  {
    other._M_data = nullptr;
    other._M_size = 0;
    other._M_used = 0;
  }

  inline buffer::~buffer()
  {
    if (_M_data) {
      free(_M_data);
    }
  }

  inline buffer& buffer::operator=(buffer&& other)
  {
    _M_data = other._M_data;
    _M_size = other._M_size;
    _M_used = other._M_used;

    other._M_data = nullptr;
    other._M_size = 0;
    other._M_used = 0;

    return *this;
  }

  inline void buffer::swap(buffer& other)
  {
    uint8_t* data = _M_data;
    _M_data = other._M_data;
    other._M_data = data;

    size_t s = _M_size;
    _M_size = other._M_size;
    other._M_size = s;

    s = _M_used;
    _M_used = other._M_used;
    other._M_used = s;
  }

  inline void buffer::clear()
  {
    _M_used = 0;
  }

  inline const void* buffer::data() const
  {
    return _M_data;
  }

  inline size_t buffer::length() const
  {
    return _M_used;
  }

  inline bool buffer::empty() const
  {
    return (_M_used == 0);
  }

  inline size_t buffer::capacity() const
  {
    return _M_size;
  }

  inline size_t buffer::remaining() const
  {
    return _M_size - _M_used;
  }

  inline bool buffer::append(const buffer& buf)
  {
    return append(buf.data(), buf.length());
  }

  inline bool buffer::append(const void* buf, size_t n)
  {
    if (n > 0) {
      if (reserve(n)) {
        memcpy(_M_data + _M_used, buf, n);
        _M_used += n;

        return true;
      }

      return false;
    }

    return true;
  }

  inline bool buffer::append(size_t n, uint8_t c)
  {
    if (n > 0) {
      if (reserve(n)) {
        memset(_M_data + _M_used, c, n);
        _M_used += n;

        return true;
      }

      return false;
    }

    return true;
  }

  inline bool buffer::push_back(uint8_t c)
  {
    if (reserve(1)) {
      _M_data[_M_used++] = c;
      return true;
    }

    return false;
  }

  inline bool buffer::insert(size_t pos, const buffer& buf)
  {
    return insert(pos, buf, 0, buf.length());
  }

  inline bool buffer::insert(size_t pos1,
                             const buffer& buf,
                             size_t pos2,
                             size_t n)
  {
    if (pos2 <= buf.length()) {
      const size_t remaining = buf.length() - pos2;

      if (n > remaining) {
        n = remaining;
      }

      return insert(pos1, buf._M_data + pos2, n);
    }

    return false;
  }

  inline bool buffer::replace(size_t pos, size_t n, const buffer& buf)
  {
    return replace(pos, n, buf.data(), buf.length());
  }

  inline bool buffer::replace(size_t pos1,
                              size_t n1,
                              const buffer& buf,
                              size_t pos2,
                              size_t n2)
  {
    if (pos2 <= buf.length()) {
      const size_t remaining = buf.length() - pos2;

      if (n2 > remaining) {
        n2 = remaining;
      }

      return replace(pos1, n1, buf._M_data + pos2, n2);
    }

    return false;
  }
}

#endif // STRING_BUFFER_H
