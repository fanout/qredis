QRedis
======

Author: Justin Karneges <justin@fanout.io>

Another Qt binding Redis. It wraps the Hiredis C API.

Setup
-----

To build the library and examples:

    qmake && make

If you want to provide extra config options, put them in a file named conf.pri first. For example:

    echo "LIBS += -L/path/to/hiredis" > conf.pri
    qmake && make

To include the code in your project, you can use the static library and headers, or include src.pri in your qmake project:

    include(/path/to/qredis/src/src.pri)

Note that if you do the qmake include, it's your responsibility to link to libhiredis.

Usage
-----

```c++
QRedis::Client *client = new QRedis::Client;
client->connectToServer("localhost", 6379);

QRedis::Request *req = client->createRequest();
connect(req,
    SIGNAL(readyRead(const QRedis::Reply &)),
    SLOT(req_readyRead(const QRedis::Reply &)));
connect(req, SIGNAL(error()), SLOT(req_error()));
req->set("foo", "hi from qredis");

void MyObject::req_readyRead(const QRedis::Reply &reply)
{
    QRedis::Request *req = (QRedis::Request *)sender();
    delete req;

    // SET succeeded
}

void MyObject::req_error()
{
    QRedis::Request *req = (QRedis::Request *)sender();
    delete req;
    
    // SET failed
}
```

Reconnect behavior
------------------

The Client class will automatically reconnect if disconnected from the server, so you only have to call connectToServer() once.
