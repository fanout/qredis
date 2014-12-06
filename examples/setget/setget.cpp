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
	bool firstConnect;

public slots:
	void start()
	{
		firstConnect = true;

		client = new QRedis::Client(this);
		client->connectToServer("localhost", 6379);
		connect(client, SIGNAL(connected()), SLOT(client_connected()));
		connect(client, SIGNAL(disconnected()), SLOT(client_disconnected()));
	}

	void doSet()
	{
		QRedis::Request *req = client->createRequest();
		connect(req, SIGNAL(readyRead(const QRedis::Reply &)), SLOT(req_set_readyRead(const QRedis::Reply &)));
		connect(req, SIGNAL(error()), SLOT(req_set_error()));
		printf("SET foo \"hi from qredis\"\n");
		req->set("foo", "hi from qredis");
	}

	void doGet()
	{
		QRedis::Request *req = client->createRequest();
		connect(req, SIGNAL(readyRead(const QRedis::Reply &)), SLOT(req_get_readyRead(const QRedis::Reply &)));
		connect(req, SIGNAL(error()), SLOT(req_get_error()));
		printf("GET foo\n");
		req->get("foo");
	}

signals:
	void quit();

private slots:
	void client_connected()
	{
		printf("connected\n");

		if(firstConnect)
		{
			printf("sleeping for 1 second\n");
			QTimer::singleShot(1000, this, SLOT(doSet()));
		}

		firstConnect = false;
	}

	void client_disconnected()
	{
		printf("disconnected\n");
	}

	void req_set_readyRead(const QRedis::Reply &reply)
	{
		QRedis::Request *req = (QRedis::Request *)sender();
		delete req;

		printf("got: [%s]\n", qPrintable(reply.value.toString()));

		printf("sleeping for 5 seconds. restart the redis server now to test reconnect\n");
		QTimer::singleShot(5000, this, SLOT(doGet()));
	}

	void req_set_error()
	{
		QRedis::Request *req = (QRedis::Request *)sender();
		delete req;

		printf("got error\n");
		emit quit();
	}

	void req_get_readyRead(const QRedis::Reply &reply)
	{
		QRedis::Request *req = (QRedis::Request *)sender();
		delete req;

		printf("got: [%s]\n", qPrintable(reply.value.toString()));
		emit quit();
	}

	void req_get_error()
	{
		QRedis::Request *req = (QRedis::Request *)sender();
		delete req;

		printf("got error\n");
		emit quit();
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

#include "setget.moc"
