QRedis
======

Author: Justin Karneges <justin@fanout.io>

Another Qt binding Redis. It wraps the Hiredis C API.

To build the library and examples:

    qmake && make

If you want to provide extra config options, put them in a file named conf.pri first. For example:

    echo "LIBS += -L/path/to/hiredis" > conf.pri
    qmake && make

To include the code in your project, you can use the static library and headers, or include src.pri in your qmake project:

    include(/path/to/qredis/src/src.pri)

Note that if you do the qmake include, it's your responsibility to link to libhiredis.
