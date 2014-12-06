#include <stdio.h>
#include <QCoreApplication>
#include <QTimer>
#include "qredisclient.h"
#include "qredisrequest.h"
#include "qredisreply.h"

class App : public QObject
{
	Q_OBJECT

private:
	QRedis::Client *client;
	bool firstReply;

public slots:
	void start()
	{
		client = new QRedis::Client(this);
		client->connectToServer("localhost", 6379);
		connect(client, SIGNAL(connected()), SLOT(client_connected()));
		connect(client, SIGNAL(disconnected()), SLOT(client_disconnected()));

		doSub();
	}

	void doSub()
	{
		firstReply = true;

		QRedis::Request *req = client->createRequest();
		req->setParent(this);
		connect(req, SIGNAL(readyRead(const QRedis::Reply &)), SLOT(req_readyRead(const QRedis::Reply &)));
		connect(req, SIGNAL(error()), SLOT(req_error()));
		req->start("subscribe", "test");
	}

signals:
	void quit();

private slots:
	void client_connected()
	{
		printf("connected\n");
	}

	void client_disconnected()
	{
		printf("disconnected\n");
	}

	void req_readyRead(const QRedis::Reply &reply)
	{
		if(reply.value.type() != QVariant::List)
		{
			printf("error: unexpected response type: %d\n", (int)reply.value.type());
			emit quit();
			return;
		}

		QVariantList l = reply.value.toList();

		if(firstReply)
		{
			if(l.count() != 3 && l[2].toInt() != 1)
			{
				printf("error: unexpected response\n");
				emit quit();
				return;
			}

			printf("subscribed\n");
		}
		else
		{
			printf("got: %s\n", qPrintable(l[2].toString()));
		}

		firstReply = false;
	}

	void req_error()
	{
		printf("got error, resubscribing\n");
		doSub();
	}
};

int main(int argc, char **argv)
{
	QCoreApplication qapp(argc, argv);
	App app;
	QObject::connect(&app, SIGNAL(quit()), &qapp, SLOT(quit()));
	QTimer::singleShot(0, &app, SLOT(start()));
	return qapp.exec();
}

#include "sub.moc"
