#ifndef BASE_LOGSTREAM_HPP
#define BASE_LOGSTREAM_HPP

#include <assert.h>
#include <string.h>
#include <string>
#include "NonCopyable.h"

class AsyncLogging;
const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

template <int SIZE>
class FixedBuffer: NonCopyable
{
public:
    FixedBuffer():cur_(data_)
    {}

    ~FixedBuffer() = default;

    void append(const char* buf, size_t len)
    {
        if(avail() > static_cast<int>(len))
        {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    const char* data() const
    {
        return data_;
    }

    int length() const
    {
        return static_cast<int>(cur_ - data_);
    }

    char* current()
    {
        return cur_;
    }

    int avail() const
    {
        return static_cast<int>(end() - cur_);
    }

    void add(size_t len)
    {
        cur_ += len;
    }

    void reset()
    {
        cur_ = data_;
    }

    void bzero()
    {
        memset(data_, 0, sizeof (data_));
    }
private:
    const char* end() const
    {
        return data_ + sizeof (data_);
    }
    char data_[SIZE];
    char* cur_;
};

class LogStream:NonCopyable
{
public:
    using Buffer = FixedBuffer<kSmallBuffer>;

    LogStream& operator<<(bool v)
    {
        buffer_.append(v ? "1" : "0", 1);
    }

    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(const void*);

    LogStream& operator<<(float v)
    {
        operator<<(static_cast<double>(v));
        return *this;
    }

    LogStream& operator<<(double v);
    LogStream& operator<<(long double v);

    LogStream& operator<<(char v)
    {
        buffer_.append(&v, 1);
    }

    LogStream& operator<<(const char* str)
    {
        if(str)
        {
            buffer_.append(str, strlen(str));
        }
        else
        {
            buffer_.append("((null))", 6);
        }

        return *this;
    }


    LogStream& operator<<(const unsigned char* v)
    {
        return operator<<(reinterpret_cast<const char*>(v));
    }


    LogStream& operator<<(const std::string& v)
    {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }

    void append(const char* data, int len)
    {
        buffer_.append(data, len);

    }

    const Buffer& buffer() const
    {
        return buffer_;
    }

    void resetBuffer()
    {
        buffer_.reset();
    }
private:
    void staticCheck();

    template<typename T>
    void formatInterger(T);

    Buffer buffer_;

    static const int kMaxNumbericSize = 32;
};

#endif // BASE_LOGSTREAM_HPP