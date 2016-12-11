#include <QtTest/QtTest>
#include <QElapsedTimer>
#include "qredisclient.h"
#include "qredisrequest.h"
#include "qredisreply.h"

Q_DECLARE_METATYPE(QRedis::Reply)

class RedisTest : public QObject
{
	Q_OBJECT

private:
	QRedis::Client *client;

        void wait(int ms)
	{
		QElapsedTimer timer;
		timer.start();
		while(true)
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents, ms);
			ms -= timer.elapsed();
			if(ms <= 0)
				break;
		}
	}

	void waitForSignal(QSignalSpy *spy)
	{
		while(spy->isEmpty())
			wait(10);
	}

	QRedis::Reply waitForReply(QRedis::Request *r)
	{
		QSignalSpy spy(r, SIGNAL(readyRead(const QRedis::Reply &)));
		waitForSignal(&spy);

		return spy.takeFirst().first().value<QRedis::Reply>();
	}

private slots:
	void initTestCase()
	{
		qRegisterMetaType<QRedis::Reply>();

		client = new QRedis::Client(this);
		client->connectToServer("localhost", 6379);

		QSignalSpy spy(client, SIGNAL(connected()));
		waitForSignal(&spy);
	}

	void cleanupTestCase()
	{
		delete client;

		QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
	}

	void setGet()
	{
		QRedis::Request *req = client->createRequest();
		req->set("test-key1", "qredis-test-string");
		QRedis::Reply rep = waitForReply(req);
		delete req;

		req = client->createRequest();
		req->get("test-key1");
		rep = waitForReply(req);
		delete req;
		QCOMPARE(rep.value.toString(), QLatin1String("qredis-test-string"));

		req = client->createRequest();
		req->del("test-key1");
		rep = waitForReply(req);
		delete req;
		QCOMPARE(rep.value.toInt(), 1);
	}
};

QTEST_MAIN(RedisTest)
#include "redistest.moc"
