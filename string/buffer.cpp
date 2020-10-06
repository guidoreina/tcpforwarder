#include "string/buffer.h"

bool string::buffer::reserve(size_t n)
{
  size_t s = _M_used + n;

  // If `s` doesn't overflow...
  if (s >= n) {
    // If we don't have to reallocate memory...
    if (s <= _M_size) {
      return true;
    }

    n = s;

    if (_M_size > 0) {
      if ((s = _M_size * 2) <= _M_size) {
        // Overflow.
        return false;
      }
    } else {
      s = initial_size;
    }

    while (s < n) {
      const size_t tmp = s * 2;
      if (tmp > s) {
        s = tmp;
      } else {
        // Overflow.
        return false;
      }
    }

    uint8_t* data = static_cast<uint8_t*>(realloc(_M_data, s));
    if (data) {
      _M_data = data;
      _M_size = s;

      return true;
    }
  }

  return false;
}

bool string::buffer::resize(size_t n)
{
  if (n > _M_used) {
    const size_t diff = n - _M_used;

    if (reserve(diff)) {
      _M_used += diff;
    } else {
      return false;
    }
  } else {
    _M_used = n;
  }

  return true;
}

bool string::buffer::append(const buffer& buf, size_t pos, size_t n)
{
  if (pos < buf.length()) {
    if (n > 0) {
      const size_t remaining = buf.length() - pos;

      if (n > remaining) {
        n = remaining;
      }

      if (reserve(n)) {
        memcpy(_M_data + _M_used, buf._M_data + pos, n);
        _M_used += n;
      } else {
        return false;
      }
    }
  } else if (pos > buf.length()) {
    return false;
  }

  return true;
}

bool string::buffer::insert(size_t pos, const void* buf, size_t n)
{
  if (pos < _M_used) {
    if (n > 0) {
      if (reserve(n)) {
        // Shift data to the right.
        memmove(_M_data + pos + n, _M_data + pos, _M_used - pos);

        memcpy(_M_data + pos, buf, n);

        _M_used += n;
      } else {
        return false;
      }
    }

    return true;
  } else if (pos == _M_used) {
    return append(buf, n);
  }

  return false;
}

bool string::buffer::insert(size_t pos, size_t n, uint8_t c)
{
  if (pos < _M_used) {
    if (n > 0) {
      if (reserve(n)) {
        // Shift data to the right.
        memmove(_M_data + pos + n, _M_data + pos, _M_used - pos);

        memset(_M_data + pos, c, n);

        _M_used += n;
      } else {
        return false;
      }
    }

    return true;
  } else if (pos == _M_used) {
    return append(n, c);
  }

  return false;
}

bool string::buffer::erase(size_t pos, size_t n)
{
  if (pos < _M_used) {
    if (n > 0) {
      const size_t remaining = _M_used - pos;

      if (n < remaining) {
        // Shift data to the left.
        memmove(_M_data + pos, _M_data + pos + n, _M_used - (pos + n));

        _M_used -= n;
      } else {
        _M_used = pos;
      }
    }
  } else if (pos > _M_used) {
    return false;
  }

  return true;
}

bool string::buffer::replace(size_t pos, size_t n1, const void* buf, size_t n2)
{
  if (pos < _M_used) {
    const size_t remaining = _M_used - pos;

    if (n1 > remaining) {
      n1 = remaining;
    }

    if (n1 < n2) {
      const size_t diff = n2 - n1;

      if (reserve(diff)) {
        // Shift data to the right.
        memmove(_M_data + pos + n2, _M_data + pos + n1, _M_used - (pos + n1));

        memcpy(_M_data + pos, buf, n2);

        _M_used += diff;
      } else {
        return false;
      }
    } else {
      memcpy(_M_data + pos, buf, n2);

      if (n1 > n2) {
        // Shift data to the left.
        memmove(_M_data + pos + n2, _M_data + pos + n1, _M_used - (pos + n1));

        _M_used -= (n1 - n2);
      }
    }

    return true;
  } else if (pos == _M_used) {
    return append(buf, n2);
  }

  return false;
}

bool string::buffer::replace(size_t pos, size_t n1, size_t n2, uint8_t c)
{
  if (pos < _M_used) {
    const size_t remaining = _M_used - pos;

    if (n1 > remaining) {
      n1 = remaining;
    }

    if (n1 < n2) {
      const size_t diff = n2 - n1;

      if (reserve(diff)) {
        // Shift data to the right.
        memmove(_M_data + pos + n2, _M_data + pos + n1, _M_used - (pos + n1));

        memset(_M_data + pos, c, n2);

        _M_used += diff;
      } else {
        return false;
      }
    } else {
      memset(_M_data + pos, c, n2);

      if (n1 > n2) {
        // Shift data to the left.
        memmove(_M_data + pos + n2, _M_data + pos + n1, _M_used - (pos + n1));

        _M_used -= (n1 - n2);
      }
    }

    return true;
  } else if (pos == _M_used) {
    return append(n2, c);
  }

  return false;
}
