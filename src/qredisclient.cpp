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

#include "qredisclient.h"

#include <assert.h>
#include <stdarg.h>
#include <QHash>
#include <QTime>
#include <QElapsedTimer>
#include <QTimer>
#include <QMutex>
#include "redisqtadapter.h"
#include "qredisrequest.h"

namespace QRedis {

class GlobalContext
{
public:
	QMutex m;
	QHash<const redisAsyncContext*, void*> contextMap;
};

Q_GLOBAL_STATIC(GlobalContext, g_context)

class Client::Private : public QObject
{
	Q_OBJECT

public:
	Client *q;
	QString host;
	int port;
	bool active;
	RedisQtAdapter *adapter;
	redisAsyncContext *ac;
	redisAsyncContext *oldAc;
	QTimer *reconnectTimer;
	QElapsedTimer time;

	Private(Client *_q) :
		QObject(_q),
		q(_q),
		active(false),
		adapter(0),
		ac(0),
		oldAc(0)
	{
		reconnectTimer = new QTimer(this);
		connect(reconnectTimer, SIGNAL(timeout()), SLOT(reconnect_timeout()));
		reconnectTimer->setSingleShot(true);
		reconnectTimer->setInterval(1000);
	}

	~Private()
	{
		cleanup();

		reconnectTimer->disconnect(this);
		reconnectTimer->setParent(0);
		reconnectTimer->deleteLater();
	}

	static void contextMapAdd(const redisAsyncContext *ac, Private *cp)
	{
		QMutexLocker locker(&(g_context()->m));
		g_context()->contextMap.insert(ac, cp);
	}

	static void contextMapRemove(const redisAsyncContext *ac)
	{
		QMutexLocker locker(&(g_context()->m));
		g_context()->contextMap.remove(ac);
	}

	static Private *contextMapGet(const redisAsyncContext *ac)
	{
		QMutexLocker locker(&(g_context()->m));
		return (Private *)g_context()->contextMap.value(ac);
	}

	void cleanup()
	{
		if(ac)
		{
			redisAsyncFree(ac);
			oldAc = ac;
			ac = 0;
		}

		delete adapter;
		adapter = 0;

		if(oldAc)
		{
			contextMapRemove(oldAc);
			oldAc = 0;
		}
	}

	void connectToServer(const QString &_host, int _port)
	{
		assert(!active);

		active = true;
		host = _host;
		port = _port;

		time.start();

		doConnect();
	}

	void logDebug(const char *fmt, va_list ap)
	{
		QString str;
		str.vsprintf(fmt, ap);

		QTime t(0, 0);
		t = t.addMSecs(time.elapsed());
		QString tstr = t.toString("HH:mm:ss.zzz");

		printf("%s %s\n", qPrintable(tstr), qPrintable(str));
	}

private:
	void doConnect()
	{
		assert(!ac);

		ac = redisAsyncConnect(host.toUtf8(), port);
		assert(ac);

		contextMapAdd(ac, this);

		if(ac->err != 0)
		{
			handleConnect(REDIS_ERR);
			return;
		}

		adapter = new RedisQtAdapter(this);
		adapter->setContext(ac);

		redisAsyncSetConnectCallback(ac, cb_connected);
		redisAsyncSetDisconnectCallback(ac, cb_disconnected);
	}

	static void cb_connected(const redisAsyncContext *c, int status)
	{
		Private *self = contextMapGet(c);
		assert(self);

		self->cb_connected(status);
	}

	static void cb_disconnected(const redisAsyncContext *c, int status)
	{
		Private *self = contextMapGet(c);
		assert(self);

		self->cb_disconnected(status);
	}

	void cb_connected(int status)
	{
		if(status == REDIS_ERR)
		{
			// hiredis will free ac after this method returns, but we need to remember
			//   the pointer for cleanup

			// only do the switch if we haven't yet
			if(ac)
			{
				oldAc = ac;
				ac = 0;
			}
		}

		QMetaObject::invokeMethod(this, "handleConnect", Qt::QueuedConnection, Q_ARG(int, status));
	}

	void cb_disconnected(int status)
	{
		// if we were explicitly disconnecting, don't react to this callback
		if(status == REDIS_OK)
			return;

		// hiredis will free ac after this method returns, but we need to remember
		//   the pointer for cleanup

		// only do the switch if we haven't yet
		if(ac)
		{
			oldAc = ac;
			ac = 0;
		}

		QMetaObject::invokeMethod(this, "handleDisconnect", Qt::QueuedConnection, Q_ARG(int, status));
	}

private slots:
	void handleConnect(int status)
	{
		if(status == REDIS_ERR)
		{
			cleanup();
			reconnectTimer->start();
			return;
		}

		emit q->connected();
	}

	void handleDisconnect(int status)
	{
		cleanup();

		if(status == REDIS_ERR)
			reconnectTimer->start();

		emit q->disconnected();
	}

	void reconnect_timeout()
	{
		doConnect();
	}
};

Client::Client(QObject *parent) :
	QObject(parent)
{
	d = new Private(this);
}

Client::~Client()
{
	delete d;
}

void Client::connectToServer(const QString &host, int port)
{
	d->connectToServer(host, port);
}

Request *Client::createRequest()
{
	Request *req = new Request;
	req->setup(this);
	return req;
}

redisAsyncContext *Client::getContext()
{
	return d->ac;
}

void Client::logDebug(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	d->logDebug(fmt, ap);
	va_end(ap);
}

}

#include "qredisclient.moc"
