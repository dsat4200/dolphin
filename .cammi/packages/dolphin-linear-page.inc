// Shared linear-tab adapter. Included in dolphintabpage.cpp so the upstream
// view and navigator lifetimes remain owned by Dolphin, not an IPC controller.
#include <QResizeEvent>
#include <QTimer>

void DolphinTabPage::rewireLeaves()
{
    for (auto container : {m_primaryViewContainer.data(), m_secondaryViewContainer.data()}) {
        if (!container) {
            continue;
        }
        disconnect(container, nullptr, this, nullptr);
        disconnect(container->view(), nullptr, this, nullptr);
        connect(container->view(), &DolphinView::activated, this, &DolphinTabPage::slotViewActivated);
        connect(container->view(), &DolphinView::toggleActiveViewRequested, this, &DolphinTabPage::switchActiveView);
        connect(container->view(), &DolphinView::redirection, this, &DolphinTabPage::slotViewUrlRedirection);
        // Inactive-pane navigation/caption changes must refresh its label too.
        connect(container, &DolphinViewContainer::captionChanged, this, [this]() {
            Q_EMIT activeViewUrlChanged(activeViewContainer()->url());
        });
    }
    if (activeViewContainer()) {
        connect(activeViewContainer()->view(), &DolphinView::urlChanged, this, &DolphinTabPage::activeViewUrlChanged);
    }
}

DolphinViewContainer *DolphinTabPage::releaseLeaf(bool primary)
{
    auto navigators = m_navigatorsWidget;
    if (navigators) {
        disconnectNavigators();
    }
    DolphinViewContainer *container = primary ? m_primaryViewContainer.data() : m_secondaryViewContainer.data();
    Q_ASSERT(container);
    disconnect(container, nullptr, this, nullptr);
    disconnect(container->view(), nullptr, this, nullptr);
    container->disconnectUrlNavigator();
    container->setActive(false);
    container->setParent(nullptr);
    if (primary) {
        m_primaryViewContainer = m_secondaryViewContainer;
    }
    m_secondaryViewContainer = nullptr;
    m_primaryViewActive = true;
    m_splitViewEnabled = false;
    rewireLeaves();
    if (navigators && m_primaryViewContainer) {
        connectNavigators(navigators);
        navigators->setSecondaryNavigatorVisible(false);
    }
    return container;
}

void DolphinTabPage::adoptSecondaryLeaf(DolphinViewContainer *container)
{
    Q_ASSERT(!m_splitViewEnabled && container);
    auto navigators = m_navigatorsWidget;
    if (navigators) {
        disconnectNavigators();
    }
    m_secondaryViewContainer = container;
    m_splitViewEnabled = true;
    m_primaryViewActive = true;
    m_splitter->addWidget(container);
    container->show();
    rewireLeaves();
    adaptSplitOrientation();
    m_splitter->setSizes({1, 1});
    if (navigators) {
        if (!navigators->secondaryUrlNavigator()) {
            navigators->createSecondaryUrlNavigator();
        }
        connectNavigators(navigators);
        navigators->setSecondaryNavigatorVisible(true);
    }
}

void DolphinTabPage::adaptSplitOrientation()
{
    if (!m_splitViewEnabled) {
        return;
    }
    // Use the whole content splitter, not either resized leaf. Primary stays
    // left/top, secondary right/bottom; the tab row is never rotated.
    const auto orientation = m_splitter->height() > m_splitter->width() ? Qt::Vertical : Qt::Horizontal;
    if (m_splitter->orientation() != orientation) {
        m_splitter->setOrientation(orientation);
        m_splitter->setSizes({1, 1});
    }
}

void DolphinTabPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Layout geometry settles after QWidget's resize; no polling or reloads.
    QTimer::singleShot(0, this, &DolphinTabPage::adaptSplitOrientation);
}
