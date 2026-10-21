// Included in dolphintabwidget.cpp. Real DolphinViewContainers (and their
// histories, selections and scroll state) move between pages without reloads.
void DolphinTabWidget::activateLeaf(int index, bool secondary)
{
    auto page = tabPageAt(index);
    if (!page) {
        return;
    }
    setCurrentIndex(index);
    const bool primary = !secondary || !page->splitViewEnabled();
    if (page->primaryViewActive() != primary) {
        page->switchActiveView();
    }
    tabBar()->update();
}

void DolphinTabWidget::splitNextLeaf()
{
    auto page = currentTabPage();
    if (!page || page->splitViewEnabled()) {
        return;
    }
    const int index = currentIndex();
    auto next = tabPageAt(index + 1);
    if (next && next->splitViewEnabled()) {
        return;
    }
    if (!next) {
        next = openNewTab(page->primaryViewContainer()->url(), QUrl(), NewTabPosition::AfterCurrent);
    }
    auto leaf = next->releaseLeaf(true);
    leaf->setProperty("linearTabLabel", next->customLabel());
    removeTab(index + 1);
    next->deleteLater();
    page->adoptSecondaryLeaf(leaf);
    page->disconnectNavigators();
    m_lastViewedTab = nullptr;
    currentTabChanged(index);
    updateTabName(index);
    tabBar()->update();
}

void DolphinTabWidget::unsplitLeaves()
{
    auto page = currentTabPage();
    if (!page || !page->splitViewEnabled()) {
        return;
    }
    const bool primaryFocused = page->primaryViewActive();
    const int index = currentIndex();
    auto leaf = page->releaseLeaf(false);
    auto next = new DolphinTabPage(leaf->url(), QUrl(), this, leaf);
    next->setCustomLabel(leaf->property("linearTabLabel").toString());
    next->setActive(false);
    connect(next, &DolphinTabPage::activeViewChanged, this, &DolphinTabWidget::activeViewChanged);
    connect(next, &DolphinTabPage::activeViewUrlChanged, this, &DolphinTabWidget::tabUrlChanged);
    insertTab(index + 1, next, QIcon(), tabName(next));
    // releaseLeaf disconnected the current page's navigators; force the same
    // native reconnect path even if the focused primary page did not change.
    page->disconnectNavigators();
    m_lastViewedTab = nullptr;
    setCurrentIndex(primaryFocused ? index : index + 1);
    currentTabChanged(currentIndex());
    updateTabName(index);
    tabBar()->update();
}

void DolphinTabWidget::moveLinearGroup(int delta)
{
    const int index = currentIndex();
    if (index + delta >= 0 && index + delta < count()) {
        tabBar()->moveTab(index, index + delta);
    }
}

void DolphinTabWidget::closeLinearGroup(int index)
{
    activateTab(index);
    if (currentTabPage()->splitViewEnabled()) {
        unsplitLeaves();
        closeTab(index + 1);
    }
    closeTab(index);
}
