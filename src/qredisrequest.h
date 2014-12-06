/*
 * Copyright (C) 2014 Justin Karneges
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef QREDISREQUEST_H
#define QREDISREQUEST_H

#include <QObject>

namespace QRedis {

class Client;
class Reply;

class Request : public QObject
{
	Q_OBJECT

public:
	~Request();

	void set(const QByteArray &key, const QByteArray &value);
	void get(const QByteArray &key);

	void start(const QByteArray &arg0,
		const QByteArray &arg1 = QByteArray(),
		const QByteArray &arg2 = QByteArray(),
		const QByteArray &arg3 = QByteArray(),
		const QByteArray &arg4 = QByteArray(),
		const QByteArray &arg5 = QByteArray(),
		const QByteArray &arg6 = QByteArray(),
		const QByteArray &arg7 = QByteArray(),
		const QByteArray &arg8 = QByteArray(),
		const QByteArray &arg9 = QByteArray());

	void start(const QList<QByteArray> &args);

signals:
	void readyRead(const QRedis::Reply &reply);
	void error();

private:
	Q_DISABLE_COPY(Request)

	friend class Client;
	Request(QObject *parent = 0);
	void setup(Client *client);

	class Private;
	friend class Private;
	Private *d;
};

}

#endif
