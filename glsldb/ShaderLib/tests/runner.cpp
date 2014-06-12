/*
 * runner.cpp
 *
 *  Created on: 10.06.2014.
 */

#include <QtCore>
#include <QtTest/qtest.h>
#include "utils/dbgprint.h"

class TestRunner {
public:
	TestRunner(int _argc, char **_argv) : argc(_argc), argv(_argv) {}
	~TestRunner() { foreach(QObject *test, tests) delete test; }

	template<typename T>
	void addTest(QString &name)
	{ tests[name] = new T(); }

	int run()
	{
		int status = 0;
		QMapIterator<QString, QObject *> iter(tests);

		while (iter.hasNext()) {
			iter.next();
			dbgPrint(DBGLVL_INFO, "Running tests %s", QTest::toString(iter.key()));
			if ((status = QTest::qExec(iter.value(), argc, argv)))
				break;
		}
		return status;
	}

protected:
	int argc;
	char **argv;
	QMap<QString, QObject *> tests;
};



int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	TestRunner runner(argc, argv);
	//runner.addTest();
	return runner.run();
}

