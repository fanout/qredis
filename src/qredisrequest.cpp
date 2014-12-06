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

#include "qredisrequest.h"

#include <assert.h>
#include <hiredis/async.h>
#include <QVariant>
#include "qredisclient.h"
#include "qredisreply.h"

namespace QRedis {

static QVariant redisReplyToVariant(const redisReply *reply)
{
	if(reply->type == REDIS_REPLY_STATUS || reply->type == REDIS_REPLY_ERROR || reply->type == REDIS_REPLY_STRING)
	{
		return QByteArray(reply->str, reply->len);
	}
	else if(reply->type == REDIS_REPLY_INTEGER)
	{
		return reply->integer;
	}
	else if(reply->type == REDIS_REPLY_NIL)
	{
		return QVariant();
	}
	else if(reply->type == REDIS_REPLY_ARRAY)
	{
		QVariantList out;
		for(int n = 0; n < (int)reply->elements; ++n)
			out += redisReplyToVariant(reply->element[n]);
		return out;
	}
	else
	{
		// unknown type!
		assert(0);
	}
}

class Request::Private : public QObject
{
	Q_OBJECT

public:
	class CommandItem
	{
	public:
		Private *rp;
	};

	Request *q;
	Client *client;
	bool active;
	bool streaming;
	CommandItem *commandItem;
	QList<QByteArray> args;
	Reply reply;

	Private(Request *_q) :
		QObject(_q),
		q(_q),
		active(false),
		streaming(false),
		commandItem(0)
	{
	}

	~Private()
	{
		if(commandItem)
			commandItem->rp = 0;
	}

	void start(const QList<QByteArray> &_args)
	{
		assert(!active);

		active = true;
		args = _args;

		if(!sendCommand())
		{
			// wait until client is connected
			connect(client, SIGNAL(connected()), SLOT(client_connected()));
		}
	}

	// return false if ac not available (between reconnects)
	bool sendCommand()
	{
		redisAsyncContext *ac = client->getContext();
		if(!ac)
			return false;

		const char **argv = (const char **)malloc(args.count() * sizeof(char *));
		size_t *argvlen = (size_t *)malloc(args.count() * sizeof(size_t));

		for(int n = 0; n < args.count(); ++n)
		{
			const QByteArray &arg = args[n];
			assert(!arg.isNull());
			argv[n] = arg.data();
			argvlen[n] = arg.length();
		}

		commandItem = new CommandItem;
		commandItem->rp = this;

		if(qstrnicmp(args[0].data(), "SUBSCRIBE", 9) == 0 ||
			qstrnicmp(args[0].data(), "PSUBSCRIBE", 10) == 0)
		{
			streaming = true;
		}

		int ret = redisAsyncCommandArgv(ac, cb_command, commandItem, args.count(), argv, argvlen);
		assert(ret == REDIS_OK);

		free(argvlen);
		free(argv);

		return true;
	}

private:
	static void cb_command(redisAsyncContext *c, void *reply, void *privdata)
	{
		Q_UNUSED(c);

		CommandItem *ci = (CommandItem *)privdata;
		Private *rp = ci->rp;
		if(!rp)
		{
			delete ci;
			return;
		}

		rp->cb_command(reply);
	}

	void cb_command(void *_reply)
	{
		if(!streaming || !_reply)
		{
			// remove callback association if not a streaming command, or we received
			//   an error while streaming
			delete commandItem;
			commandItem = 0;
		}

		if(_reply)
		{
			reply.value = redisReplyToVariant((const redisReply *)_reply);

			QMetaObject::invokeMethod(this, "handleReply", Qt::QueuedConnection);
		}
		else
			QMetaObject::invokeMethod(this, "handleError", Qt::QueuedConnection);
	}

private slots:
	void client_connected()
	{
		disconnect(client, SIGNAL(connected()), this, SLOT(client_connected()));

		sendCommand();
	}

	void handleReply()
	{
		if(!streaming)
			active = false;

		// emit a copy from the stack, so the request is deletable
		Reply r = reply;
		emit q->readyRead(r);
	}

	void handleError()
	{
		active = false;
		emit q->error();
	}
};

Request::Request(QObject *parent) :
	QObject(parent)
{
	d = new Private(this);
}

Request::~Request()
{
	delete d;
}

void Request::set(const QByteArray &key, const QByteArray &value)
{
	d->start(QList<QByteArray>() << "SET" << key << value);
}

void Request::get(const QByteArray &key)
{
	d->start(QList<QByteArray>() << "GET" << key);
}

#define TRY_ADD_ARG(a) if(!a.isNull()) { args += a; } else { goto end; }

void Request::start(const QByteArray &arg0,
	const QByteArray &arg1,
    const QByteArray &arg2,
    const QByteArray &arg3,
    const QByteArray &arg4,
    const QByteArray &arg5,
    const QByteArray &arg6,
    const QByteArray &arg7,
    const QByteArray &arg8,
    const QByteArray &arg9)
{
	QList<QByteArray> args;
	args += arg0;

	TRY_ADD_ARG(arg1)
	TRY_ADD_ARG(arg2)
	TRY_ADD_ARG(arg3)
	TRY_ADD_ARG(arg4)
	TRY_ADD_ARG(arg5)
	TRY_ADD_ARG(arg6)
	TRY_ADD_ARG(arg7)
	TRY_ADD_ARG(arg8)
	TRY_ADD_ARG(arg9)

end:
	start(args);
}

void Request::start(const QList<QByteArray> &args)
{
	d->start(args);
}

void Request::setup(Client *client)
{
	d->client = client;
}

}

#include "qredisrequest.moc"
