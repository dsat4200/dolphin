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
