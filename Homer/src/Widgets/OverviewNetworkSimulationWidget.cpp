/*****************************************************************************
 *
 * Copyright (C) 2011 Thomas Volkert <thomas@homer-conferencing.com>
 *
 * This software is free software.
 * Your are allowed to redistribute it and/or modify it under the terms of
 * the GNU General Public License version 2 as published by the Free Software
 * Foundation.
 *
 * This source is published in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License version 2
 * along with this program. Otherwise, you can write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 * Alternatively, you find an online version of the license text under
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 *****************************************************************************/

/*
 * Purpose: Implementation of OverviewNetworkSimulationWidget.h
 * Author:  Thomas Volkert
 * Since:   2012-06-03
 */

#define DEBUG_TOPOLOGY_CREATION
//#define DEBUG_GUI_SIMULATION_TIMING

#include <Core/Coordinator.h>
#include <Core/Scenario.h>
#include <Dialogs/ContactEditDialog.h>
#include <Widgets/OverviewNetworkSimulationWidget.h>
#include <MainWindow.h>
#include <ContactsPool.h>
#include <Configuration.h>
#include <Logger.h>

#include <QFont>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsLineItem>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QTableWidgetItem>
#include <QStyledItemDelegate>
#include <QDockWidget>
#include <QModelIndex>
#include <QHostInfo>
#include <QPoint>
#include <QFileDialog>

namespace Homer { namespace Gui {

///////////////////////////////////////////////////////////////////////////////

static struct StreamDescriptor sEmptyStreamDesc = {0, {0, 0, 0}, "", "", 0, 0};

///////////////////////////////////////////////////////////////////////////////

class HierarchyItem:
    public QStandardItem
{
public:
    HierarchyItem(QString pText, Coordinator *pCoordinator, Node* pNode):QStandardItem()
    {
        mCoordinator = pCoordinator;
        mNode = pNode;
        setText(pText);
        //LOG(LOG_VERBOSE, "Creating hierarchy item %s with %d children", pText.toStdString().c_str(), pChildCount);
    }
    ~HierarchyItem(){ }

    Coordinator* GetCoordinator(){ return mCoordinator; }
    Node* GetNode(){ return mNode; }
private:
    Coordinator *mCoordinator;
    Node* mNode;
};

///////////////////////////////////////////////////////////////////////////////

class StreamItem:
    public QStandardItem
{
public:
    StreamItem(QString pText, struct StreamDescriptor pStreamDesc):QStandardItem(pText), mText(pText)
    {
        mStreamDesc = pStreamDesc;
        //LOG(LOG_VERBOSE, "Creating stream item %s", pText.toStdString().c_str());
    }
    ~StreamItem(){ }

    struct StreamDescriptor GetDesc(){ return mStreamDesc; }
    QString GetText(){ return mText; }

private:
    struct StreamDescriptor mStreamDesc;
    QString                 mText;
};

///////////////////////////////////////////////////////////////////////////////

GuiNode::GuiNode(Node* pNode, OverviewNetworkSimulationWidget* pNetSimWidget):
    QGraphicsPixmapItem(QPixmap(":/images/46_46/Hardware.png"), NULL)
{
    mNode = pNode;
    mNetSimWidget = pNetSimWidget;
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    #ifdef DEBUG_TOPOLOGY_CREATION
        LOG(LOG_WARN, "Created GUI node %s", pNode->GetAddress().c_str());
    #endif

    // create GUI node text object (uses node's address)
    mTextItem = new QGraphicsTextItem(QString(pNode->GetAddress().c_str()), this);
    mTextItem->setPos(0, 45);
    mTextItem->setDefaultTextColor(Qt::black);
    mTextItem->setFont(QFont("Arial", 9, QFont::Normal, false));
}
GuiNode::~GuiNode()
{

}

int GuiNode::type() const
{
    return GUI_NODE_TYPE;
}

QVariant GuiNode::itemChange(GraphicsItemChange pChange, const QVariant &pValue)
{
    switch(pChange)
    {
        case QGraphicsItem::ItemPositionChange:
            GuiLink* tGuiLink;
            foreach (tGuiLink, mGuiLinks)
            {
                tGuiLink->UpdatePosition();
            }
            break;
        case QGraphicsItem::ItemSelectedChange:
            if (pValue.toBool())
            {
                mTextItem->setFont(QFont("Arial", 9, QFont::Bold, false));
                mTextItem->setDefaultTextColor(Qt::red);
                mNetSimWidget->UpdateRoutingView();
            }else
            {
                mTextItem->setFont(QFont("Arial", 9, QFont::Normal, false));
                mTextItem->setDefaultTextColor(Qt::black);
            }
            break;
        default:
            break;
    }

    return QGraphicsItem::itemChange(pChange, pValue);
}

void GuiNode::AddGuiLink(GuiLink *pGuiLink)
{
    mGuiLinks.append(pGuiLink);
}

int GuiNode::GetWidth()
{
    return (int)boundingRect().width();
}

int GuiNode::GetHeight()
{
    return (int)boundingRect().height();
}

Node* GuiNode::GetNode()
{
    return mNode;
}

///////////////////////////////////////////////////////////////////////////////

GuiLink::GuiLink(GuiNode* pGuiNode0, GuiNode* pGuiNode1, QWidget* pParent):QGraphicsLineItem(), mGuiNode0(pGuiNode0), mGuiNode1(pGuiNode1), mParent(pParent)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    setZValue(-1000.0);
    pGuiNode0->AddGuiLink(this);
    pGuiNode1->AddGuiLink(this);
    UpdatePosition();
    #ifdef DEBUG_TOPOLOGY_CREATION
        LOG(LOG_WARN, "Created GUI link between %s and %s", pGuiNode0->GetNode()->GetAddress().c_str(), pGuiNode1->GetNode()->GetAddress().c_str());
    #endif
}
GuiLink::~GuiLink()
{

}

int GuiLink::type() const
{
    return GUI_LINK_TYPE;
}

void GuiLink::UpdatePosition()
{
    QPointF tPoint0 = mapFromItem(mGuiNode0, 0, 0);
    tPoint0.setX(tPoint0.x() + mGuiNode0->GetWidth() / 2);
    tPoint0.setY(tPoint0.y() + mGuiNode0->GetHeight() / 2);
    QPointF tPoint1 = mapFromItem(mGuiNode1, 0, 0);
    tPoint1.setX(tPoint1.x() + mGuiNode1->GetWidth() / 2);
    tPoint1.setY(tPoint1.y() + mGuiNode1->GetHeight() / 2);
    //LOG(LOG_VERBOSE, "Line from %d,%d to %d,%d", (int)tPoint0.x(), (int)tPoint0.y(), (int)tPoint1.x(), (int)tPoint1.y());
    QLineF tLine(tPoint0, tPoint1);
    setLine(tLine);
}

GuiNode* GuiLink::GetGuiNode0()
{
    return mGuiNode0;
}
GuiNode* GuiLink::GetGuiNode1()
{
    return mGuiNode1;
}

///////////////////////////////////////////////////////////////////////////////

NetworkScene::NetworkScene(OverviewNetworkSimulationWidget *pNetSimWidget):
    QGraphicsScene(pParent)
{
    mNetSimWidget = pNetSimWidget;
    connect(this, SIGNAL(selectionChanged()), mNetSimWidget, SLOT(SelectedNewNetworkItem()));
    mScaleFactor = 1.0;
}

NetworkScene::~NetworkScene()
{

}

void NetworkScene::wheelEvent(QGraphicsSceneWheelEvent *pEvent)
{
    int tOffset = pEvent->delta() * 5 / 120;
    LOG(LOG_VERBOSE, "Got new wheel event with delta %d, results in offset: %d", pEvent->delta(), tOffset);

    mNetSimWidget->NetworkViewZoomChanged((mScaleFactor * 100.0) + tOffset);
}

void NetworkScene::Scale(qreal pFactor)
{
    QGraphicsView *tView = views().first();
    mScaleFactor = pFactor;
    QMatrix tOldMatrix = tView->matrix();
    tView->resetMatrix();
    tView->translate(tOldMatrix.dx(), tOldMatrix.dy());
    tView->scale(pFactor, pFactor);
}

///////////////////////////////////////////////////////////////////////////////

OverviewNetworkSimulationWidget::OverviewNetworkSimulationWidget(QAction *pAssignedAction, QMainWindow* pMainWindow, Scenario *pScenario):
    QDockWidget(pMainWindow)
{
    mAssignedAction = pAssignedAction;
    mMainWindow = pMainWindow;
    mScenario = pScenario;
    mStreamRow = 0;
    mHierarchyRow = 0;
    mHierarchyCol = 0;
    mHierarchyEntry = NULL;
    mSelectedNode = NULL;

    initializeGUI();

    setAllowedAreas(Qt::AllDockWidgetAreas);
    pMainWindow->addDockWidget(Qt::TopDockWidgetArea, this);

    if (mAssignedAction != NULL)
    {
        connect(mAssignedAction, SIGNAL(triggered(bool)), this, SLOT(SetVisible(bool)));
        mAssignedAction->setChecked(true);
    }
    connect(toggleViewAction(), SIGNAL(toggled(bool)), mAssignedAction, SLOT(setChecked(bool)));
    connect(mTvHierarchy, SIGNAL(clicked(QModelIndex)), this, SLOT(SelectedCoordinator(QModelIndex)));
    connect(mTvStreams, SIGNAL(clicked(QModelIndex)), this, SLOT(SelectedStream(QModelIndex)));
    connect(mSlZoom, SIGNAL(valueChanged (int)), this, SLOT(NetworkViewZoomChanged(int)));

    SetVisible(CONF.GetVisibilityNetworkSimulationWidget());
    mAssignedAction->setChecked(CONF.GetVisibilityNetworkSimulationWidget());

    InitNetworkView();
    UpdateHierarchyView();
    UpdateStreamsView();
}

OverviewNetworkSimulationWidget::~OverviewNetworkSimulationWidget()
{
    CONF.SetVisibilityNetworkSimulationWidget(isVisible());
    delete mTvHierarchyModel;
    delete mTvStreamsModel;
}

///////////////////////////////////////////////////////////////////////////////

void OverviewNetworkSimulationWidget::initializeGUI()
{
    setupUi(this);

    mTvHierarchyModel = new QStandardItemModel(this);
    mTvHierarchy->setModel(mTvHierarchyModel);
    mTvStreamsModel = new QStandardItemModel(this);
    mTvStreams->setModel(mTvStreamsModel);

    mNetworkScene = new NetworkScene(this);
    mGvNetwork->setScene(mNetworkScene);
}

void OverviewNetworkSimulationWidget::closeEvent(QCloseEvent* pEvent)
{
    SetVisible(false);
}

void OverviewNetworkSimulationWidget::SetVisible(bool pVisible)
{
    if (pVisible)
    {
        move(mWinPos);
        show();
        // update GUI elements every x ms
        mTimerId = startTimer(NETWORK_SIMULATION_GUI_UPDATE_TIME);
    }else
    {
        if (mTimerId != -1)
            killTimer(mTimerId);
        mWinPos = pos();
        hide();
    }
}

void OverviewNetworkSimulationWidget::contextMenuEvent(QContextMenuEvent *pEvent)
{
//    QAction *tAction;
//
//    QMenu tMenu(this);
//
//    tAction = tMenu.addAction("Add contact");
//    QIcon tIcon1;
//    tIcon1.addPixmap(QPixmap(":/images/22_22/Plus.png"), QIcon::Normal, QIcon::Off);
//    tAction->setIcon(tIcon1);
//
//    QAction* tPopupRes = tMenu.exec(pEvent->globalPos());
//    if (tPopupRes != NULL)
//    {
//        if (tPopupRes->text().compare("Add contact") == 0)
//        {
//            InsertNew();
//            return;
//        }
//    }
}

void OverviewNetworkSimulationWidget::timerEvent(QTimerEvent *pEvent)
{
    #ifdef DEBUG_GUI_SIMULATION_TIMING
        LOG(LOG_VERBOSE, "New timer event");
    #endif
    if (pEvent->timerId() == mTimerId)
    {
        UpdateStreamsView();
        UpdateNetworkView();
        UpdateRoutingView();
    }
}

void OverviewNetworkSimulationWidget::SelectedCoordinator(QModelIndex pIndex)
{
    if(!pIndex.isValid())
        return;

    int tCurHierarchyRow = pIndex.row();
    int tCurHierarchyCol = pIndex.column();
    void *tCurHierarchyEntry = pIndex.internalPointer();
    if (tCurHierarchyEntry != NULL)
        tCurHierarchyEntry = ((QStandardItem*)tCurHierarchyEntry)->child(tCurHierarchyRow, tCurHierarchyCol);

    HierarchyItem *tItem = (HierarchyItem*)tCurHierarchyEntry;

    if (tItem != NULL)
    {
        if (tItem->GetCoordinator() != NULL)
        {
            LOG(LOG_VERBOSE, "User selected coordinator in row: %d and col.: %d, coordinator: %s", tCurHierarchyRow, tCurHierarchyCol, tItem->GetCoordinator()->GetClusterAddress().c_str());
        }else if (tItem->GetNode() != NULL)
        {
            LOG(LOG_VERBOSE, "User selected node in row: %d and col.: %d, node: %s", tCurHierarchyRow, tCurHierarchyCol, tItem->GetNode()->GetAddress().c_str());
        }else
        {
            LOG(LOG_VERBOSE, "User selected coordinator in row: %d and col.: %d, internal pointer: %p", tCurHierarchyRow, tCurHierarchyCol, pIndex.internalPointer());
        }
    }

    if ((tCurHierarchyRow != mHierarchyRow) || (tCurHierarchyCol != mHierarchyCol) || (tCurHierarchyEntry != mHierarchyEntry))
    {
        LOG(LOG_VERBOSE, "Update of hierarchy view needed");
        mHierarchyRow = tCurHierarchyRow;
        mHierarchyCol = tCurHierarchyCol;
        mHierarchyEntry = tCurHierarchyEntry;
        ShowHierarchyDetails((tItem->GetCoordinator() != NULL) ? tItem->GetCoordinator() : NULL, tItem->GetNode() ? tItem->GetNode() : NULL);
    }
}

void OverviewNetworkSimulationWidget::SelectedStream(QModelIndex pIndex)
{
    if(!pIndex.isValid())
        return;

    int tCurStreamRow = pIndex.row();
    LOG(LOG_VERBOSE, "User selected stream in row: %d", tCurStreamRow);

    if(tCurStreamRow != mStreamRow)
    {
        LOG(LOG_VERBOSE, "Update of streams view needed");
        mStreamRow = tCurStreamRow;
    }
}

void OverviewNetworkSimulationWidget::NetworkViewZoomChanged(int pZoom)
{
    qreal tScaleFactor = (qreal)pZoom / 100.0;

    if ((pZoom < 0.25) || (pZoom > 1.75))
        return;

    if (mSlZoom->value() != pZoom)
        mSlZoom->setValue(pZoom);

    mNetworkScene->Scale(tScaleFactor);
}

void OverviewNetworkSimulationWidget::ShowHierarchyDetails(Coordinator *pCoordinator, Node* pNode)
{
    if (pCoordinator != NULL)
    {
        mLbHierarchyLevel->setText(QString("%1").arg(pCoordinator->GetHierarchyLevel()));
        mLbSiblings->setText(QString("%1").arg(pCoordinator->GetSiblings().size()));
        if (pCoordinator->GetHierarchyLevel() != 0)
            mLbChildren->setText(QString("%1").arg(pCoordinator->GetChildCoordinators().size()));
        else
            mLbChildren->setText(QString("%1").arg(pCoordinator->GetClusterMembers().size()));
    }else
    {
        if (pNode != NULL)
        {
            mLbHierarchyLevel->setText("node");
            mLbSiblings->setText(QString("%1").arg(pNode->GetSiblings().size()));
            mLbChildren->setText("0");
        }else
        {
            mLbHierarchyLevel->setText("-");
            mLbSiblings->setText("-");
            mLbChildren->setText("-");
        }
    }
}

void OverviewNetworkSimulationWidget::UpdateHierarchyView()
{
    Coordinator *tRootCoordinator = mScenario->GetRootCoordinator();
    mTvHierarchyModel->clear();
    QStandardItem *tRootItem = mTvHierarchyModel->invisibleRootItem();

    // create root entry of the tree
    HierarchyItem *tRootCoordinatorItem = new HierarchyItem("Root: " + QString(tRootCoordinator->GetClusterAddress().c_str()), tRootCoordinator, NULL);
    tRootItem->appendRow(tRootCoordinatorItem);

    // recursive creation of tree items
    AppendHierarchySubItems(tRootCoordinatorItem);

    // show all entries of the tree
    mTvHierarchy->expandAll();

    if (mHierarchyEntry == NULL)
        ShowHierarchyDetails(NULL, NULL);
}

void OverviewNetworkSimulationWidget::AppendHierarchySubItems(HierarchyItem *pParentItem)
{
    Coordinator *tParentCoordinator = pParentItem->GetCoordinator();

    if (tParentCoordinator->GetHierarchyLevel() == 0)
    {// end of tree reached
        NodeList tNodeList = tParentCoordinator->GetClusterMembers();
        NodeList::iterator tIt;
        int tCount = 0;
        for (tIt = tNodeList.begin(); tIt != tNodeList.end(); tIt++)
        {
            HierarchyItem* tItem = new HierarchyItem("Node  " + QString((*tIt)->GetAddress().c_str()), NULL, *tIt);
            pParentItem->setChild(tCount, tItem);
            tCount++;
        }
    }else
    {// further sub trees possible
        CoordinatorList tCoordinatorList = tParentCoordinator->GetChildCoordinators();
        CoordinatorList::iterator tIt;
        QList<QStandardItem*> tItems;
        int tCount = 0;
        for (tIt = tCoordinatorList.begin(); tIt != tCoordinatorList.end(); tIt++)
        {
            HierarchyItem *tItem = new HierarchyItem("Coord. " + QString((*tIt)->GetClusterAddress().c_str()), *tIt, NULL);
            pParentItem->setChild(tCount, tItem);
            //LOG(LOG_VERBOSE, "Appending coordinator %s", tItem->GetCoordinator()->GetClusterAddress().c_str());
            AppendHierarchySubItems(tItem);
            tCount++;
        }
    }
}

// #####################################################################
// ############ streams view
// #####################################################################
void OverviewNetworkSimulationWidget::ShowStreamDetails(const struct StreamDescriptor pDesc)
{
    mLbPackets->setText(QString("%1").arg(pDesc.PacketCount));
    mLbDataRate->setText(QString("%1").arg(pDesc.QoSRequs.DataRate));
    mLbDelay->setText(QString("%1").arg(pDesc.QoSRequs.Delay));
}

QString OverviewNetworkSimulationWidget::CreateStreamId(const struct StreamDescriptor pDesc)
{
    return QString(pDesc.LocalNode.c_str()) + ":" + QString("%1").arg(pDesc.LocalPort) + " <==> " +QString(pDesc.PeerNode.c_str()) + ":" + QString("%1").arg(pDesc.PeerPort);
}

void OverviewNetworkSimulationWidget::UpdateStreamsView()
{
    bool tResetNeeded = false;

    //LOG(LOG_VERBOSE, "Updating streams view");

    StreamList tStreams = mScenario->GetStreams();
    StreamList::iterator tIt;
    QStandardItem *tRootItem = mTvStreamsModel->invisibleRootItem();

    if (tStreams.size() == 0)
    {
        ShowStreamDetails(sEmptyStreamDesc);
        // if the QTreeView is already empty we can return immediately
        if (tRootItem->rowCount() == 0)
            return;
    }

    int tCount = 0;
    // check if update is need
    if (tRootItem->rowCount() != 0)
    {
        for(tIt = tStreams.begin(); tIt != tStreams.end(); tIt++)
        {
            QString tDesiredString  = CreateStreamId(*tIt);
            StreamItem* tCurItem = (StreamItem*) mTvStreamsModel->item(tCount, 0);
            if (tCurItem != NULL)
            {
                LOG(LOG_WARN, "Comparing %s and %s", tDesiredString.toStdString().c_str(), tCurItem->GetText().toStdString().c_str());
                if (tDesiredString != tCurItem->GetText())
                    tResetNeeded = true;
            }else
                tResetNeeded = true;
            if (tCount == mStreamRow)
                ShowStreamDetails(*tIt);
            tCount++;
        }
    }else
        tResetNeeded = true;

    // update the entire view
    if (tResetNeeded)
    {
        tCount = 0;
        mTvStreamsModel->clear();
        tRootItem = mTvStreamsModel->invisibleRootItem();
        if (tRootItem == NULL)
        {
            LOG(LOG_WARN, "Root item is invalid");
            tRootItem = new QStandardItem();
            mTvStreamsModel->appendRow(tRootItem);
        }
        for(tIt = tStreams.begin(); tIt != tStreams.end(); tIt++)
        {
            StreamItem* tItem = new StreamItem(CreateStreamId(*tIt), *tIt);
            tRootItem->appendRow(tItem);
            if (tCount == mStreamRow)
                ShowStreamDetails(*tIt);
            tCount++;
        }
    }
}

// #####################################################################
// ############ network view
// #####################################################################
GuiNode* OverviewNetworkSimulationWidget::GetGuiNode(Node *pNode)
{
    GuiNode *tGuiNode;
    foreach(tGuiNode, mGuiNodes)
    {
        if (tGuiNode->GetNode()->GetAddress() == pNode->GetAddress())
        {
            return tGuiNode;
        }
    }

    LOG(LOG_ERROR, "Cannot determine GuiNode object for the given node %s", pNode->GetAddress().c_str());

    return NULL;
}

void OverviewNetworkSimulationWidget::InitNetworkView()
{
    // create all node GUI elements
    NodeList tNodes = mScenario->GetNodes();
    NodeList::iterator tIt;
    for (tIt = tNodes.begin(); tIt != tNodes.end(); tIt++)
    {
        // create GUI node object
        GuiNode *tGuiNode = new GuiNode(*tIt, this);
        mGuiNodes.append(tGuiNode);
        mNetworkScene->addItem(tGuiNode);
        tGuiNode->setPos((*tIt)->GetPosXHint(), (*tIt)->GetPosYHint());
        LOG(LOG_VERBOSE, "Set pos. of node %s to %d,%d", (*tIt)->GetAddress().c_str(), (*tIt)->GetPosXHint(), (*tIt)->GetPosYHint());
    }

    // create all link GUI elements
    LinkList tLinks = mScenario->GetLinks();
    LinkList::iterator tIt2;
    for (tIt2 = tLinks.begin(); tIt2 != tLinks.end(); tIt2++)
    {
        GuiLink *tGuiLink = new GuiLink(GetGuiNode((*tIt2)->GetNode0()), GetGuiNode((*tIt2)->GetNode1()), this);
        mGuiLinks.append(tGuiLink);
        mNetworkScene->addItem(tGuiLink);
    }
}

void OverviewNetworkSimulationWidget::UpdateNetworkView()
{
//    GuiLink *tGuiLink;
//    foreach(tGuiLink, mGuiLinks)
//    {
//        tGuiLink->UpdatePosition();
//    }
}

// #####################################################################
// ############ routing view
// #####################################################################
void OverviewNetworkSimulationWidget::SelectedNewNetworkItem()
{
    QList<QGraphicsItem*> tItems = mNetworkScene->selectedItems();
    QList<QGraphicsItem*>::iterator tIt;
    for (tIt = tItems.begin(); tIt != tItems.end(); tIt++)
    {
        // our way of "reflections": show routing table of first selected node
        if ((*tIt)->type() == GUI_NODE_TYPE)
        {
            LOG(LOG_VERBOSE, "New node item selected");
            GuiNode *tSelectedGuiNode = (GuiNode*)tItems.first();
            mSelectedNode = tSelectedGuiNode->GetNode();
            break;
        }
        if ((*tIt)->type() == GUI_LINK_TYPE)
        {
            LOG(LOG_VERBOSE, "New link item selected");
        }
    }
}

void OverviewNetworkSimulationWidget::FillRoutingTableCell(int pRow, int pCol, QString pText)
{
    if (mTwRouting->item(pRow, pCol) != NULL)
        mTwRouting->item(pRow, pCol)->setText(pText);
    else
    {
        QTableWidgetItem *tItem =  new QTableWidgetItem(pText);
        tItem->setTextAlignment(Qt::AlignCenter|Qt::AlignVCenter);
        mTwRouting->setItem(pRow, pCol, tItem);
    }
}

void OverviewNetworkSimulationWidget::FillRoutingTableRow(int pRow, RibEntry* pEntry)
{
    if (pRow > mTwRouting->rowCount() - 1)
        mTwRouting->insertRow(mTwRouting->rowCount());

    FillRoutingTableCell(pRow, 0, QString(pEntry->Destination.c_str()));
    FillRoutingTableCell(pRow, 1, QString(pEntry->NextNode.c_str()));
    FillRoutingTableCell(pRow, 2, QString("%1").arg(pEntry->HopCount));
    FillRoutingTableCell(pRow, 3, QString("%1").arg(pEntry->QoSCapabilities.DataRate));
    FillRoutingTableCell(pRow, 4, QString("%1").arg(pEntry->QoSCapabilities.Delay));
}

void OverviewNetworkSimulationWidget::UpdateRoutingView()
{
    NodeList tNodes = mScenario->GetNodes();

    if ((mSelectedNode == NULL) && (tNodes.size() > 0))
    {
        // select first in list
        mSelectedNode = *tNodes.begin();
    }

    if (mSelectedNode == NULL)
        return;

    mGrpRouting->setTitle(" Routing table " + QString(mSelectedNode->GetAddress().c_str()));
    RibTable tRib = mSelectedNode->GetRib();
    RibTable::iterator tIt;
    int tRow = 0;
    for (tIt = tRib.begin(); tIt != tRib.end(); tIt++)
    {
        FillRoutingTableRow(tRow++, *tIt);
    }

    for (int i = mTwRouting->rowCount(); i > tRow; i--)
        mTwRouting->removeRow(i);

    mTwRouting->setRowCount(tRow);
}

///////////////////////////////////////////////////////////////////////////////

}} //namespace
