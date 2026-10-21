/*
 * SPDX-FileCopyrightText: 2014 Emmanuel Pescosta <emmanuelpescosta099@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphintabbar.h"
#include "dolphin_generalsettings.h"
#include "dolphintabwidget.h"
#include "dolphinviewcontainer.h"
#include <KLocalizedString>

#include <QDragEnterEvent>
#include <QInputDialog>
#include <QMenu>
#include <QMimeData>
#include <QTimer>

class PreventFocusWhileHidden : public QObject
{
public:
    PreventFocusWhileHidden(QObject *parent)
        : QObject(parent){};

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        switch (ev->type()) {
        case QEvent::Hide:
            static_cast<QWidget *>(obj)->setFocusPolicy(Qt::NoFocus);
            return false;
        case QEvent::Show:
            static_cast<QWidget *>(obj)->setFocusPolicy(Qt::TabFocus);
            return false;
        default:
            return false;
        }
    };
};

DolphinTabBar::DolphinTabBar(QWidget *parent)
    : QTabBar(parent)
    , m_autoActivationIndex(-1)
    , m_tabToBeClosedOnMiddleMouseButtonRelease(-1)
{
    setAcceptDrops(true);
    setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    setMovable(true);
    setTabsClosable(true);

    setFocusPolicy(Qt::NoFocus);
    installEventFilter(new PreventFocusWhileHidden(this));

    m_autoActivationTimer = new QTimer(this);
    m_autoActivationTimer->setSingleShot(true);
    m_autoActivationTimer->setInterval(800);
    connect(m_autoActivationTimer, &QTimer::timeout, this, &DolphinTabBar::slotAutoActivationTimeout);
    connect(GeneralSettings::self(), &GeneralSettings::tabBarChanged, this, &DolphinTabBar::slotTabBarChanged);

    QTimer::singleShot(0, this, &DolphinTabBar::slotTabBarChanged);
}

QSize DolphinTabBar::tabSizeHint(int index) const
{
    if (GeneralSettings::tabStyle() == GeneralSettings::EnumTabStyle::FixedSize) {
        QSize defaultSize = QTabBar::tabSizeHint(index);
        defaultSize.setWidth(225);
        return defaultSize;
    } else if (GeneralSettings::tabStyle() == GeneralSettings::EnumTabStyle::FullWidth && count() > 0) {
        QSize defaultSize = QTabBar::tabSizeHint(index);
        defaultSize.setWidth(qMax(25, width() / count()));
        return defaultSize;
    }
    return QTabBar::tabSizeHint(index);
}

QSize DolphinTabBar::minimumSizeHint() const
{
    QSize s = QTabBar::minimumSizeHint();

    if (GeneralSettings::tabStyle() == GeneralSettings::EnumTabStyle::FullWidth) {
        s.setWidth(0); // allow shrinking
    }

    return s;
}

void DolphinTabBar::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    const int index = tabAt(event->position().toPoint());

    if (mimeData->hasUrls()) {
        event->acceptProposedAction();
        updateAutoActivationTimer(index);
    }

    QTabBar::dragEnterEvent(event);
}

void DolphinTabBar::dragLeaveEvent(QDragLeaveEvent *event)
{
    updateAutoActivationTimer(-1);

    QTabBar::dragLeaveEvent(event);
}

void DolphinTabBar::dragMoveEvent(QDragMoveEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    const int index = tabAt(event->position().toPoint());

    if (mimeData->hasUrls()) {
        Q_EMIT tabDragMoveEvent(index, event);
        updateAutoActivationTimer(index);
    }

    QTabBar::dragMoveEvent(event);
}

void DolphinTabBar::dropEvent(QDropEvent *event)
{
    // Disable the auto activation timer
    updateAutoActivationTimer(-1);

    const QMimeData *mimeData = event->mimeData();
    const int index = tabAt(event->position().toPoint());

    if (mimeData->hasUrls()) {
        Q_EMIT tabDropEvent(index, event);
    }

    QTabBar::dropEvent(event);
}

void DolphinTabBar::mousePressEvent(QMouseEvent *event)
{
    const int index = tabAt(event->pos());

    if (index >= 0 && event->button() == Qt::MiddleButton) {
        if (auto widget = qobject_cast<DolphinTabWidget *>(parentWidget())) {
            widget->activateLeaf(index, event->position().x() >= tabRect(index).center().x());
        }
        m_tabToBeClosedOnMiddleMouseButtonRelease = index;
        return;
    }

    QTabBar::mousePressEvent(event);
    if (index >= 0 && (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)) {
        if (auto widget = qobject_cast<DolphinTabWidget *>(parentWidget())) {
            widget->activateLeaf(index, event->position().x() >= tabRect(index).center().x());
        }
    }
}

void DolphinTabBar::mouseReleaseEvent(QMouseEvent *event)
{
    const int index = tabAt(event->pos());

    if (index >= 0 && index == m_tabToBeClosedOnMiddleMouseButtonRelease && event->button() == Qt::MiddleButton) {
        // Mouse middle click on a tab closes this tab.
        Q_EMIT tabCloseRequested(index);
        return;
    }

    QTabBar::mouseReleaseEvent(event);
}

void DolphinTabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        int index = tabAt(event->pos());

        if (index < 0) {
            // empty tabbar area case
            index = currentIndex();
        }
        // Double left click on the tabbar opens a new activated tab
        // with the url from the doubleclicked tab or currentTab otherwise.
        Q_EMIT openNewActivatedTab(index);
    }

    QTabBar::mouseDoubleClickEvent(event);
}

void DolphinTabBar::contextMenuEvent(QContextMenuEvent *event)
{
    const int index = tabAt(event->pos());

    if (index >= 0) {
        // Tab context menu
        QMenu menu(this);

        QAction *newTabAction = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-new")), i18nc("@action:inmenu", "New Tab"));
        QAction *detachTabAction = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-detach")), i18nc("@action:inmenu", "Detach Tab"));
        QAction *closeOtherTabsAction = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-close-other")), i18nc("@action:inmenu", "Close Other Tabs"));
        QAction *closeTabAction = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-close")), i18nc("@action:inmenu", "Close Tab"));

        QAction *renameTabAction = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-rename")), i18nc("@action:inmenu", "Rename Tab"));

        QAction *selectedAction = menu.exec(event->globalPos());
        if (selectedAction == newTabAction) {
            Q_EMIT openNewActivatedTab(index);
        } else if (selectedAction == detachTabAction) {
            Q_EMIT tabDetachRequested(index);
        } else if (selectedAction == closeOtherTabsAction) {
            if (auto widget = qobject_cast<DolphinTabWidget *>(parentWidget())) {
                const auto target = widget->tabPageAt(index);
                while (widget->indexOf(target) > 0) {
                    widget->closeLinearGroup(0);
                }
                while (widget->count() > widget->indexOf(target) + 1) {
                    widget->closeLinearGroup(widget->count() - 1);
                }
            }
        } else if (selectedAction == closeTabAction) {
            Q_EMIT tabCloseRequested(index);
        } else if (selectedAction == renameTabAction) {
            bool renamed = false;
            const QString tabNewName = QInputDialog::getText(this, i18nc("@title:window for text input", "Rename Tab"), i18n("New tab name:"), QLineEdit::Normal, tabText(index), &renamed);

            if (renamed) {
                Q_EMIT tabRenamed(index, tabNewName);
            }
        }

        return;
    }

    QTabBar::contextMenuEvent(event);
}

void DolphinTabBar::slotTabBarChanged()
{
    if (GeneralSettings::tabStyle() == GeneralSettings::EnumTabStyle::FixedSize) {
        setExpanding(false);
        setUsesScrollButtons(true);
    } else if (GeneralSettings::tabStyle() == GeneralSettings::EnumTabStyle::FullWidth) {
        setExpanding(true);
        setUsesScrollButtons(false);
    } else {
        setExpanding(false);
        setUsesScrollButtons(true);
    }

    updateGeometry();
}

void DolphinTabBar::slotAutoActivationTimeout()
{
    if (m_autoActivationIndex >= 0) {
        setCurrentIndex(m_autoActivationIndex);
        updateAutoActivationTimer(-1);
    }
}

void DolphinTabBar::updateAutoActivationTimer(const int index)
{
    if (m_autoActivationIndex != index) {
        m_autoActivationIndex = index;

        if (m_autoActivationIndex < 0) {
            m_autoActivationTimer->stop();
        } else {
            m_autoActivationTimer->start();
        }
    }
}

#include "moc_dolphintabbar.cpp"
// Rounded upper corners with a square folder base. One real draggable tab
// extent contains two separately painted/clickable leaf labels when paired.
#include <QPainter>
#include <QPainterPath>
#include <cmath>

static QColor linearTabText(const QColor &background)
{
    auto linear = [](double c) { return c <= .04045 ? c / 12.92 : std::pow((c + .055) / 1.055, 2.4); };
    const double luminance = .2126 * linear(background.redF()) + .7152 * linear(background.greenF()) + .0722 * linear(background.blueF());
    return (luminance + .05) / .05 >= 1.05 / (luminance + .05) ? QColor(Qt::black) : QColor(Qt::white);
}

void DolphinTabBar::paintEvent(QPaintEvent *)
{
    auto widget = qobject_cast<DolphinTabWidget *>(parentWidget());
    if (!widget) {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().window());
    for (int index = 0; index < count(); ++index) {
        const auto page = widget->tabPageAt(index);
        if (!page) {
            continue;
        }
        const int leaves = page->splitViewEnabled() ? 2 : 1;
        for (int leaf = 0; leaf < leaves; ++leaf) {
            QRect area = tabRect(index);
            const int split = area.left() + area.width() / 2;
            if (leaves == 2) {
                if (leaf == 0) {
                    area.setRight(split - 1);
                } else {
                    area.setLeft(split);
                }
            }
            const bool active = index == currentIndex() && page->primaryViewActive() == (leaf == 0);
            QColor background = active ? palette().color(QPalette::Highlight) : palette().color(QPalette::HighlightedText);
            background.setAlpha(255);
            QPainterPath shape;
            // Union the square base with the rounded rectangle; Qt's default
            // odd/even rule would cut a hole where the two shapes overlap.
            shape.setFillRule(Qt::WindingFill);
            if (active) {
                shape.addRoundedRect(area, 6, 6);
                shape.addRect(area.left(), area.bottom() - 6, area.width(), 7);
            } else {
                shape.addRect(area);
            }
            painter.fillPath(shape, background);
            painter.setPen(linearTabText(background));
            const auto container = leaf == 0 ? page->primaryViewContainer() : page->secondaryViewContainer();
            QString label = leaf == 0 ? page->customLabel() : container->property("linearTabLabel").toString();
            if (label.isEmpty()) {
                label = container->caption();
            }
            auto font = painter.font();
            font.setBold(active);
            painter.setFont(font);
            QRect textArea = area.adjusted(10, 0, -10, 0);
            // Native close buttons remain child widgets; do not draw over them.
            for (auto side : {QTabBar::LeftSide, QTabBar::RightSide}) {
                if (auto button = tabButton(index, side); button && button->geometry().intersects(textArea)) {
                    if (side == QTabBar::RightSide) {
                        textArea.setRight(button->geometry().left() - 4);
                    } else {
                        textArea.setLeft(button->geometry().right() + 4);
                    }
                }
            }
            painter.drawText(textArea, Qt::AlignVCenter | Qt::AlignLeft,
                             painter.fontMetrics().elidedText(label, Qt::ElideRight, qMax(0, textArea.width())));
        }
    }
}
