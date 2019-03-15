#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

class NonCopyable
{
public:
    NonCopyable() = default;
    ~NonCopyable() = default;
    NonCopyable(const NonCopyable& ) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
#endif // NONCOPYABLE_H
