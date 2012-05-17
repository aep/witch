#include <errno.h>
#include <stdexcept>
#include <QByteArray>

class PosixException : public std::runtime_error
{
public:
    PosixException(const char *what)
        : std::runtime_error((
                    QByteArray(what) + " : " + QByteArray(strerror(errno)))
                .data())
    {
    }
};

class PolicyException : public std::runtime_error
{
public:
    PolicyException(const char *what)
        : std::runtime_error(what)
    {
    }
};


