// SPDX-License-Identifier: GPL-2.0-or-later
#include "dolphintabwidget.h"
#include "dolphinviewcontainer.h"
#include <QApplication>
#include <QDir>
#include <QImage>
#include <QPixmap>
#include <QSignalSpy>
#include <QSplitter>
#include <QStandardPaths>
#include <QTabBar>
#include <QTemporaryDir>
#include <QTest>
#include <QVBoxLayout>

class LinearTabsTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void preservesViewsAndFlatOrder()
    {
        QStandardPaths::setTestModeEnabled(true);
        QTemporaryDir directory;
        QWidget window;
        auto layout = new QVBoxLayout(&window);
        auto navigators = new DolphinNavigatorsWidgetAction(&window);
        auto tabs = new DolphinTabWidget(navigators, &window);
        layout->addWidget(tabs);
        for (const auto name : {"A", "B", "C", "D"}) {
            QDir(directory.path()).mkdir(QString::fromLatin1(name));
            tabs->openNewActivatedTab(QUrl::fromLocalFile(directory.filePath(QString::fromLatin1(name))));
        }
        auto a = tabs->tabPageAt(0)->primaryViewContainer();
        auto b = tabs->tabPageAt(1)->primaryViewContainer();
        auto c = tabs->tabPageAt(2)->primaryViewContainer();
        auto d = tabs->tabPageAt(3)->primaryViewContainer();
        // The default insertion path, not only the shortcut, is adjacent.
        tabs->activateTab(1);
        tabs->splitNextLeaf();
        QCOMPARE(tabs->count(), 3);
        QCOMPARE(tabs->currentTabPage()->primaryViewContainer(), b);
        QCOMPARE(tabs->currentTabPage()->secondaryViewContainer(), c);
        tabs->splitNextLeaf(); // A pair cannot become a tree or steal D.
        QCOMPARE(tabs->count(), 3);
        tabs->activateLeaf(0, false);
        for (auto expected : {b, c, d, a}) {
            tabs->activateNextTab();
            QCOMPARE(tabs->currentTabPage()->activeViewContainer(), expected);
        }
        for (auto expected : {d, c, b, a}) {
            tabs->activatePrevTab();
            QCOMPARE(tabs->currentTabPage()->activeViewContainer(), expected);
        }
        tabs->activateLeaf(1, true);
        tabs->moveLinearGroup(1);
        QCOMPARE(tabs->currentIndex(), 2);
        QCOMPARE(tabs->currentTabPage()->activeViewContainer(), c);
        tabs->moveLinearGroup(1); // Movement doesn't wrap.
        QCOMPARE(tabs->currentIndex(), 2);
        tabs->moveLinearGroup(-1);
        tabs->unsplitLeaves();
        QCOMPARE(tabs->count(), 4);
        QCOMPARE(tabs->currentIndex(), 2);
        QCOMPARE(tabs->currentTabPage()->primaryViewContainer(), c);
        QCOMPARE(tabs->tabPageAt(1)->primaryViewContainer(), b);
        tabs->activateLeaf(1, false);
        tabs->splitNextLeaf();
        window.resize(1200, 600);
        window.show();
        auto splitter = tabs->currentTabPage()->findChild<DolphinTabPageSplitter *>();
        QTRY_COMPARE(splitter->orientation(), Qt::Horizontal);
        window.resize(550, 1200);
        QTRY_COMPARE(splitter->orientation(), Qt::Vertical);
        QCOMPARE(tabs->currentTabPage()->primaryViewContainer(), b);
        QCOMPARE(tabs->currentTabPage()->secondaryViewContainer(), c);
        QCOMPARE(tabs->currentTabPage()->activeViewContainer(), b);
        const auto bar = tabs->tabBar();
        const auto label = bar->tabRect(tabs->currentIndex());
        const auto image = bar->grab().toImage();
        const auto fillPoint = QPoint(label.left() + label.width() / 4, label.bottom() - 2);
        QCOMPARE(image.pixelColor(fillPoint).rgb(), bar->palette().color(QPalette::Highlight).rgb());
        QSignalSpy closed(tabs, &DolphinTabWidget::rememberClosedTab);
        tabs->activateLeaf(1, true);
        tabs->closeTab();
        QCOMPARE(tabs->count(), 3);
        QCOMPARE(tabs->tabPageAt(1)->primaryViewContainer(), b);
        QCOMPARE(closed.size(), 1);
        tabs->restoreClosedTab(closed.first().at(1).toByteArray());
        QCOMPARE(tabs->count(), 4);
        QCOMPARE(tabs->currentTabPage()->primaryViewContainer()->url(), QUrl::fromLocalFile(directory.filePath("C")));
    }
};

QTEST_MAIN(LinearTabsTest)
#include "dolphin-linear-test.moc"
