/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <QCoreApplication>
#include <QtTest>
#include <QSignalSpy>
#include <qqml.h>

#include "persistenttabmodel.h"
#include "declarativehistorymodel.h"
#include "testobject.h"
#include "dbmanager.h"
#include "browserpaths.h"

struct TabTuple {
    TabTuple(QString url, QString title) : url(url), title(title) {}
    TabTuple() {}

    QString url;
    QString title;
};

Q_DECLARE_METATYPE(TabTuple)

class tst_declarativehistorymodel : public TestObject
{
    Q_OBJECT

public:
    tst_declarativehistorymodel();

private slots:
    void initTestCase();
    void init();

    void addNonSameHistoryEntries_data();
    void addNonSameHistoryEntries();
    void addDuplicateHistoryEntries_data();
    void addDuplicateHistoryEntries();

    void sortedHistoryEntries_data();
    void sortedHistoryEntries();

    void emptyTitles_data();
    void emptyTitles();

    void removeHistoryEntries_data();
    void removeHistoryEntries();

    void searchWithSpecialChars_data();
    void searchWithSpecialChars();

    void cleanup();

private:
    void addTabs(const QList<TabTuple> &tabs);
    void verifySearchResult(QString searchTerm, int expectedCount);

    DeclarativeHistoryModel *historyModel;
    DeclarativeTabModel *tabModel;
    QString dbFileName;
};


tst_declarativehistorymodel::tst_declarativehistorymodel()
    : TestObject()
{
}

void tst_declarativehistorymodel::initTestCase()
{

    dbFileName = QString("%1/%2")
            .arg(BrowserPaths::dataLocation())
            .arg(QLatin1String(DB_NAME));
    QFile dbFile(dbFileName);
    dbFile.remove();
}

void tst_declarativehistorymodel::init()
{
    tabModel = new PersistentTabModel(DBManager::instance()->getMaxTabId() + 1);
    historyModel = new DeclarativeHistoryModel;

    QVERIFY(tabModel);
    QVERIFY(historyModel);

    if (!tabModel->loaded()) {
        QSignalSpy loadedSpy(tabModel, SIGNAL(loadedChanged()));
        // Tabs must be loaded with in 500ms
        QVERIFY(loadedSpy.wait());
        QCOMPARE(loadedSpy.count(), 1);
    }
}

void tst_declarativehistorymodel::addNonSameHistoryEntries_data()
{
    QTest::addColumn<QList<TabTuple> >("tabs");
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<int>("expectedCount");

    QList<TabTuple> list {
        TabTuple(QStringLiteral("http://www.foobar.com/page1/"), QStringLiteral("FooBar Page1")),
        TabTuple(QStringLiteral("http://www.foobar.com/page2/"), QStringLiteral("FooBar Page2")),
        TabTuple(QStringLiteral("http://www.foobar.com/page3/"), QStringLiteral("FooBar Page3"))
    };

    QTest::newRow("foo1") << list << "Page1" << 1;
    QTest::newRow("foo2") << list << "Page2" << 1;
    QTest::newRow("foo3") << list << "Page3" << 1;
}

void tst_declarativehistorymodel::addNonSameHistoryEntries()
{
    QFETCH(QList<TabTuple>, tabs);
    QFETCH(QString, searchTerm);
    QFETCH(int, expectedCount);

    addTabs(tabs);
    verifySearchResult("", tabs.count());
    verifySearchResult(searchTerm, expectedCount);
}

void tst_declarativehistorymodel::addDuplicateHistoryEntries_data()
{
    QTest::addColumn<QList<TabTuple> >("tabs");
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<int>("expectedCount");

    QList<TabTuple> list {
        TabTuple(QStringLiteral("http://www.foobar.com/page1/"), QStringLiteral("FooBar Page1")),
        TabTuple(QStringLiteral("http://www.foobar.com/page1/"), QStringLiteral("FooBar Page1")),
        TabTuple(QStringLiteral("http://www.foobar.com/page2/"), QStringLiteral("FooBar Page2")),
        TabTuple(QStringLiteral("http://www.foobar.com/page2/"), QStringLiteral("FooBar Page2")),
        TabTuple(QStringLiteral("http://www.foobar.com/page3/"), QStringLiteral("FooBar Page3")),
        TabTuple(QStringLiteral("http://www.foobar.com/page3/"), QStringLiteral("FooBar Page3"))
    };

    QTest::newRow("foo1") << list << "Page1" << 1;
    QTest::newRow("foo2") << list << "Page2" << 1;
    QTest::newRow("foo3") << list << "Page3" << 1;
}

void tst_declarativehistorymodel::addDuplicateHistoryEntries()
{
    QFETCH(QList<TabTuple>, tabs);
    QFETCH(QString, searchTerm);
    QFETCH(int, expectedCount);

    addTabs(tabs);
    verifySearchResult("", 3);
    verifySearchResult(searchTerm, expectedCount);
}

void tst_declarativehistorymodel::sortedHistoryEntries_data()
{
    QTest::addColumn<QList<TabTuple> >("tabs");
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<QStringList>("order");
    QTest::addColumn<int>("expectedCount");


    QList<TabTuple> list {
        TabTuple(QStringLiteral("http://www.testurl.blah/thelongesturl/"), QStringLiteral("The longest url")),
    };


    // Insert in reversed order
    QTest::newRow("longestUrl") << (QList<TabTuple>() <<
                                   TabTuple(QStringLiteral("http://www.testurl.blah/thelongesturl/"), QStringLiteral("The longest url"))) << "test"
                              << (QStringList() << "http://www.testurl.blah/thelongesturl/") << 1;
    QTest::newRow("longerUrl") << (QList<TabTuple>() <<
                                   TabTuple(QStringLiteral("http://www.testurl.blah/thelongesturl/"), QStringLiteral("The longest url")) <<
                                   TabTuple(QStringLiteral("http://www.testurl.blah/alongerurl/"), QStringLiteral("A longer url")))
                               << "test" << (QStringList() << "http://www.testurl.blah/alongerurl/"
                                   << "http://www.testurl.blah/thelongesturl/") << 2;

    QTest::newRow("rootPage") << (QList<TabTuple>() <<
                                  TabTuple(QStringLiteral("http://www.testurl.blah/thelongesturl/"), QStringLiteral("The longest url")) <<
                                  TabTuple(QStringLiteral("http://www.testurl.blah/alongerurl/"), QStringLiteral("A longer url")) <<
                                  TabTuple(QStringLiteral("http://www.testurl.blah/"), QStringLiteral("A root page")))
                              << "test"
                              << (QStringList() << "http://www.testurl.blah/" << "http://www.testurl.blah/alongerurl/"
                                  << "http://www.testurl.blah/thelongesturl/") << 3;
}

void tst_declarativehistorymodel::sortedHistoryEntries()
{
    QFETCH(QList<TabTuple>, tabs);
    QFETCH(QString, searchTerm);
    QFETCH(QStringList, order);
    QFETCH(int, expectedCount);

    addTabs(tabs);
    verifySearchResult(searchTerm, expectedCount);

    for (int i = 0; i < expectedCount; ++i) {
        QModelIndex modelIndex = historyModel->createIndex(i, 0);
        QString url = historyModel->data(modelIndex, DeclarativeHistoryModel::UrlRole).toString();
        QCOMPARE(url, order.at(i));
    }
}

void tst_declarativehistorymodel::emptyTitles_data()
{
    QTest::addColumn<QList<TabTuple> >("tabs");
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<int>("expectedCount");

    QTest::newRow("duplicate_longestUrl") << (QList<TabTuple>() <<
                                              TabTuple(QStringLiteral("http://www.testurl.blah/thelongesturl/"), QStringLiteral("The longest url")) <<
                                              TabTuple(QStringLiteral("http://www.testurl.blah/thelongesturl/"), QStringLiteral(""))) << "test" << 1;
    QTest::newRow("random_url") << (QList<TabTuple>() << TabTuple(QStringLiteral("http://quick"), QStringLiteral(""))) << "quick" << 0;
}

void tst_declarativehistorymodel::emptyTitles()
{
    QFETCH(QList<TabTuple>, tabs);
    QFETCH(QString, searchTerm);
    QFETCH(int, expectedCount);

    addTabs(tabs);
    verifySearchResult(searchTerm, expectedCount);

    for (int i = 0; i < expectedCount; ++i) {
        QModelIndex modelIndex = historyModel->createIndex(i, 0);
        QString title = historyModel->data(modelIndex, DeclarativeHistoryModel::TitleRole).toString();
        QVERIFY(!title.isEmpty());
    }
}

void tst_declarativehistorymodel::removeHistoryEntries_data()
{
    QStringList urls;
    urls << "http://removeTestUrl1" << "http://removeTestUrl2" << "http://removeTestUrl3";
    QStringList titles;
    titles << "test1" << "test2" << "test3";


    QList<TabTuple> list {
        TabTuple(QStringLiteral("http://removeTestUrl1"), QStringLiteral("test1")),
        TabTuple(QStringLiteral("http://removeTestUrl2"), QStringLiteral("test2")),
        TabTuple(QStringLiteral("http://removeTestUrl3"), QStringLiteral("test3")),
    };

    QTest::addColumn<QList<TabTuple> >("tabs");
    QTest::addColumn<int>("index");
    QTest::addColumn<int>("countWithSearchTermIndexRemoved");
    QTest::addColumn<int>("countWithEmptySearchIndexRemoved");
    QTest::addColumn<int>("countWithSearchTerm");
    QTest::addColumn<QString>("searchTerm");
    QTest::newRow("remove_first") << list << 0 << 0 << 2 << 1 << list.at(0).url;
    QTest::newRow("remove_middle") << list << 1 << 0 << 2 << 1  << list.at(1).url;
    QTest::newRow("remove_last") << list << 2 << 0 << 2 << 1 << list.at(2).url;
    QTest::newRow("out_of_bounds_negative") << list << -1 << 3 << 3 << 3 << "removeTestUrl";
    QTest::newRow("out_of_bounds_positive") << list << 4 << 3 << 3 << 3 << "removeTestUrl";
}

void tst_declarativehistorymodel::removeHistoryEntries()
{
    QFETCH(QList<TabTuple>, tabs);
    QFETCH(int, index);
    QFETCH(int, countWithSearchTermIndexRemoved);
    QFETCH(int, countWithEmptySearchIndexRemoved);
    QFETCH(int, countWithSearchTerm);

    QFETCH(QString, searchTerm);
    addTabs(tabs);
    verifySearchResult(searchTerm, countWithSearchTerm);
    // Reset search results.
    verifySearchResult("", tabs.count());

    historyModel->remove(index);
    verifySearchResult("", countWithEmptySearchIndexRemoved);

    historyModel->search(searchTerm);
    verifySearchResult(searchTerm, countWithSearchTermIndexRemoved);
}

void tst_declarativehistorymodel::searchWithSpecialChars_data()
{
    QTest::addColumn<QList<TabTuple> >("tabs");
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<int>("expectedCount");
    QTest::newRow("special_site") << (QList<TabTuple>() << TabTuple(QStringLiteral("http://www.pöö.com/"), QStringLiteral("wierd site"))) << "pöö" << 1;
    QTest::newRow("special_title") << (QList<TabTuple>() << TabTuple(QStringLiteral("http://www.pöö.com/"), QStringLiteral("wierd site"))
                                      << TabTuple(QStringLiteral("http://www.foobar.com/"), QStringLiteral("pöö wierd title"))) << "pöö" << 2;

    QTest::newRow("special_escaped_chars") << (QList<TabTuple>() << TabTuple(QStringLiteral("http://www.foobar.com/"), QStringLiteral("special title: ';\";ö"))) << "';\";" << 1;
    QTest::newRow("special_escaped_chars") << (QList<TabTuple>() << TabTuple(QStringLiteral("http://www.foobar.com/"), QStringLiteral("Ö is wierd char"))) << "Ö" << 1;
}

void tst_declarativehistorymodel::searchWithSpecialChars()
{
    QFETCH(QList<TabTuple>, tabs);
    QFETCH(QString, searchTerm);
    QFETCH(int, expectedCount);

    addTabs(tabs);

    verifySearchResult(searchTerm, expectedCount);

    // Wierdly this works in unit test, but in production code doesn't, perhaps linking to different sqlite version
    // QEXPECT_FAIL("special_upper_case_special_char", "due to sqlite bug accented char is case sensitive with LIKE op", Continue);
}

void tst_declarativehistorymodel::cleanup()
{
    delete tabModel;
    tabModel = 0;
    delete historyModel;
    historyModel = 0;
    delete DBManager::instance();
    QFile dbFile(dbFileName);
    QVERIFY(dbFile.remove());
}

void tst_declarativehistorymodel::addTabs(const QList<TabTuple> &tabs)
{
    QSignalSpy tabCountChangeSpy(tabModel, SIGNAL(countChanged()));
    for (int i = 0; i < tabs.count(); ++i) {
        tabModel->addTab(tabs.at(i).url, tabs.at(i).title, tabModel->count());
    }

    waitSignals(tabCountChangeSpy, tabs.count());
}

void tst_declarativehistorymodel::verifySearchResult(QString searchTerm, int expectedCount)
{
    QSignalSpy countChangeSpy(historyModel, SIGNAL(countChanged()));
    QSignalSpy historyAvailable(DBManager::instance(), SIGNAL(historyAvailable(QList<Link>)));
    historyModel->search(searchTerm);
    waitSignals(countChangeSpy, 1, 300);
    waitSignals(historyAvailable, 1, 500);
    QCOMPARE(historyModel->rowCount(), expectedCount);
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);
    tst_declarativehistorymodel testcase;
    return QTest::qExec(&testcase, argc, argv); \
}
#include "tst_declarativehistorymodel.moc"
