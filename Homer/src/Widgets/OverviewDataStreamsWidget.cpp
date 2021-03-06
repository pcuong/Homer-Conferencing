/*****************************************************************************
 *
 * Copyright (C) 2009 Thomas Volkert <thomas@homer-conferencing.com>
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
 * Purpose: Implementation of OverviewDataStreamsWidget.h
 * Author:  Thomas Volkert
 * Since:   2009-04-20
 */

#include <Widgets/OverviewDataStreamsWidget.h>
#include <PacketStatisticService.h>
#include <Configuration.h>
#include <Snippets.h>
#include <QDockWidget>
#include <QMainWindow>
#include <QTimerEvent>
#include <QHeaderView>
#include <QFileDialog>
#include <QSizePolicy>
#include <QScrollBar>
#include <QMenu>
#include <QContextMenuEvent>

namespace Homer { namespace Gui {

using namespace Homer::Monitor;
using namespace Homer::Base;

///////////////////////////////////////////////////////////////////////////////

OverviewDataStreamsWidget::OverviewDataStreamsWidget(QAction *pAssignedAction, QMainWindow* pMainWindow):
    QDockWidget(pMainWindow)
{
    mAssignedAction = pAssignedAction;
    mTimerId = -1;

    initializeGUI();

    setAllowedAreas(Qt::AllDockWidgetAreas);
    pMainWindow->addDockWidget(Qt::TopDockWidgetArea, this);

    if (mAssignedAction != NULL)
    {
        connect(mAssignedAction, SIGNAL(triggered(bool)), this, SLOT(SetVisible(bool)));
        mAssignedAction->setChecked(true);
    }
    connect(toggleViewAction(), SIGNAL(toggled(bool)), mAssignedAction, SLOT(setChecked(bool)));
    connect(mTwVideo, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(TwVideoCustomContextMenuEvent(const QPoint&)));
    connect(mTwAudio, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(TwAudioCustomContextMenuEvent(const QPoint&)));
    if (CONF.DebuggingEnabled())
    {
        SetVisible(CONF.GetVisibilityDataStreamsWidget());
        mAssignedAction->setChecked(CONF.GetVisibilityDataStreamsWidget());
    }else
    {
        SetVisible(false);
        mAssignedAction->setChecked(false);
        mAssignedAction->setVisible(false);
        toggleViewAction()->setVisible(false);
    }
}

OverviewDataStreamsWidget::~OverviewDataStreamsWidget()
{
	CONF.SetVisibilityDataStreamsWidget(isVisible());
}

///////////////////////////////////////////////////////////////////////////////

void OverviewDataStreamsWidget::initializeGUI()
{
    setupUi(this);

    // hide id column
    mTwAudio->setColumnHidden(12, true);
    mTwAudio->sortItems(12);
    mTwAudio->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
    mTwAudio->horizontalHeader()->resizeSection(0, mTwAudio->horizontalHeader()->sectionSize(0) * 3);
    for (int i = 1; i < 5; i++)
        mTwAudio->horizontalHeader()->resizeSection(i, mTwAudio->horizontalHeader()->sectionSize(i) * 2);
    mTwAudio->horizontalHeader()->resizeSection(7, (int)mTwAudio->horizontalHeader()->sectionSize(7) * 2);
    mTwAudio->horizontalHeader()->resizeSection(10, mTwAudio->horizontalHeader()->sectionSize(9) * 2);
    mTwAudio->horizontalHeader()->resizeSection(11, mTwAudio->horizontalHeader()->sectionSize(10) * 2);

    // hide id column
    mTwVideo->setColumnHidden(12, true);
    mTwVideo->sortItems(12);
    mTwVideo->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
    mTwVideo->horizontalHeader()->resizeSection(0, mTwVideo->horizontalHeader()->sectionSize(0) * 3);
    for (int i = 1; i < 5; i++)
        mTwVideo->horizontalHeader()->resizeSection(i, mTwVideo->horizontalHeader()->sectionSize(i) * 2);
    mTwVideo->horizontalHeader()->resizeSection(7, (int)mTwVideo->horizontalHeader()->sectionSize(7) * 2);
    mTwVideo->horizontalHeader()->resizeSection(10, mTwVideo->horizontalHeader()->sectionSize(9) * 2);
    mTwVideo->horizontalHeader()->resizeSection(11, mTwVideo->horizontalHeader()->sectionSize(10) * 2);

    UpdateView();
}

void OverviewDataStreamsWidget::closeEvent(QCloseEvent* pEvent)
{
    SetVisible(false);
}

void OverviewDataStreamsWidget::SetVisible(bool pVisible)
{
	CONF.SetVisibilityDataStreamsWidget(pVisible);
    if (pVisible)
    {
        move(mWinPos);
        show();
        mTimerId = startTimer(VIEW_DATA_STREAMS_UPDATE_PERIOD);
    }else
    {
        if (mTimerId != -1)
            killTimer(mTimerId);
        mWinPos = pos();
        hide();
    }
}

void OverviewDataStreamsWidget::contextMenuEvent(QContextMenuEvent *pContextMenuEvent)
{
    QAction *tAction;

    QMenu tMenu(this);

    tAction = tMenu.addAction("Save statistic");
    QIcon tIcon1;
    tIcon1.addPixmap(QPixmap(":/images/22_22/Save.png"), QIcon::Normal, QIcon::Off);
    tAction->setIcon(tIcon1);

    QAction* tPopupRes = tMenu.exec(pContextMenuEvent->globalPos());
    if (tPopupRes != NULL)
    {
        if (tPopupRes->text().compare("Save statistic") == 0)
        {
            SaveCompleteStatistic();
            return;
        }
    }
}

void OverviewDataStreamsWidget::TwVideoCustomContextMenuEvent(const QPoint &pPos)
{
    QAction *tAction;

    QMenu tMenu(this);

    tAction = tMenu.addAction(Homer::Gui::OverviewDataStreamsWidget::tr("Save all"));
    QIcon tIcon1;
    tIcon1.addPixmap(QPixmap(":/images/22_22/Save.png"), QIcon::Normal, QIcon::Off);
    tAction->setIcon(tIcon1);

    tAction = tMenu.addAction(Homer::Gui::OverviewDataStreamsWidget::tr("Save row history"));
    tAction->setIcon(tIcon1);

    tAction = tMenu.addAction(Homer::Gui::OverviewDataStreamsWidget::tr("Reset row"));
    QIcon tIcon2;
    tIcon2.addPixmap(QPixmap(":/images/22_22/Reload.png"), QIcon::Normal, QIcon::Off);
    tAction->setIcon(tIcon2);

    QAction* tPopupRes = tMenu.exec(QCursor::pos());
    if (tPopupRes != NULL)
    {
        if (tPopupRes->text().compare(Homer::Gui::OverviewDataStreamsWidget::tr("Reset row")) == 0)
        {
            ResetStatistic(DATA_TYPE_VIDEO, mTwVideo->currentRow());
            return;
        }
        if (tPopupRes->text().compare(Homer::Gui::OverviewDataStreamsWidget::tr("Save row history")) == 0)
        {
            SaveHistory(DATA_TYPE_VIDEO, mTwVideo->currentRow());
            return;
        }
        if (tPopupRes->text().compare(Homer::Gui::OverviewDataStreamsWidget::tr("Save all")) == 0)
        {
            SaveCompleteStatistic();
            return;
        }
    }
}

void OverviewDataStreamsWidget::TwAudioCustomContextMenuEvent(const QPoint &pPos)
{
    QAction *tAction;

    QMenu tMenu(this);

    tAction = tMenu.addAction(Homer::Gui::OverviewDataStreamsWidget::tr("Save all"));
    QIcon tIcon1;
    tIcon1.addPixmap(QPixmap(":/images/22_22/Save.png"), QIcon::Normal, QIcon::Off);
    tAction->setIcon(tIcon1);

    tAction = tMenu.addAction(Homer::Gui::OverviewDataStreamsWidget::tr("Save row history"));
    tAction->setIcon(tIcon1);

    tAction = tMenu.addAction(Homer::Gui::OverviewDataStreamsWidget::tr("Reset row"));
    QIcon tIcon2;
    tIcon2.addPixmap(QPixmap(":/images/22_22/Reload.png"), QIcon::Normal, QIcon::Off);
    tAction->setIcon(tIcon2);

    QAction* tPopupRes = tMenu.exec(QCursor::pos());
    if (tPopupRes != NULL)
    {
        if (tPopupRes->text().compare(Homer::Gui::OverviewDataStreamsWidget::tr("Reset row")) == 0)
        {
            ResetStatistic(DATA_TYPE_AUDIO, mTwAudio->currentRow());
            return;
        }
        if (tPopupRes->text().compare(Homer::Gui::OverviewDataStreamsWidget::tr("Save row history")) == 0)
        {
            SaveHistory(DATA_TYPE_AUDIO, mTwAudio->currentRow());
            return;
        }
        if (tPopupRes->text().compare(Homer::Gui::OverviewDataStreamsWidget::tr("Save all")) == 0)
        {
            SaveCompleteStatistic();
            return;
        }
    }
}

void OverviewDataStreamsWidget::SaveHistory(enum DataType pDataType, int pIndex)
{
    DataRateHistory tHistory;

    int tCount = 0;
    PacketStatistics::iterator tIt;
    PacketStatistics tStatList = SVC_PACKET_STATISTIC.GetPacketStatisticsAccess();

    if (tStatList.size() > 0)
    {
        tIt = tStatList.begin();

        while (tIt != tStatList.end())
        {
            if ((*tIt)->GetDataType() == pDataType)
            {
                //### reset statistic
                if (tCount == pIndex)
                {
                    tHistory = (*tIt)->GetDataRateHistory();
                    LOG(LOG_VERBOSE, "Going to save history with %d entries for %s", tHistory.size(), (*tIt)->GetStreamName().c_str());
                    break;
                }
                tCount++;
            }

            //### next entry
            tIt++;
        }
    }

    SVC_PACKET_STATISTIC.ReleasePacketStatisticsAccess();

    if (tHistory.size() > 0)
    {
        QString tFileName = QFileDialog::getSaveFileName(this,
                                                         Homer::Gui::OverviewDataStreamsWidget::tr("Save data rate history"),
                                                         QDir::homePath() + Homer::Gui::OverviewDataStreamsWidget::tr("/DataRateHistory.csv"),
                                                         Homer::Gui::OverviewDataStreamsWidget::tr("Comma-Separated Values File") + " (*.csv)",
                                                         NULL,
                                                         CONF_NATIVE_DIALOGS);

        if (tFileName.isEmpty())
            return;

        // write history to file
        QFile tFile(tFileName);
        if (!tFile.open(QIODevice::ReadWrite))
        {
            ShowError(Homer::Gui::OverviewDataStreamsWidget::tr("Unable to open file"), Homer::Gui::OverviewDataStreamsWidget::tr("The selected output file can't be opened"));
            return;
        }

        //#####################################################
        //### write header to csv
        //#####################################################
        QString tHeader = "Time,TimeStamp,Rate\n";
        if (!tFile.write(tHeader.toStdString().c_str(), tHeader.size()))
            return;

        //#####################################################
        //### write one entry per line
        //#####################################################
        QString tLine;
        DataRateHistory::iterator tHistIt;

        tHistIt = tHistory.begin();
        while (tHistIt != tHistory.end())
        {
            //#######################
            //### calculate line
            //#######################
            tLine = QString("%1,").arg(tHistIt->Time);
            tLine += QString("%1,").arg(tHistIt->TimeStamp);
            tLine += QString("%1").arg(tHistIt->DataRate);
            tLine += "\n";

            //#######################
            //### write to file
            //#######################
            if (!tFile.write(tLine.toStdString().c_str(), tLine.size()))
                return;

            //#######################
            //### next entry
            //#######################
            tHistIt++;
        }

        tFile.close();

    }else
    {
        ShowWarning("History empty", "The history of measured data rates is empty, can't save to file");
    }
}

void OverviewDataStreamsWidget::ResetStatistic(enum DataType pDataType, int pIndex)
{
    int tCount = 0;
    PacketStatistics::iterator tIt;
    PacketStatistics tStatList = SVC_PACKET_STATISTIC.GetPacketStatisticsAccess();

    if (tStatList.size() > 0)
    {
        tIt = tStatList.begin();

        while (tIt != tStatList.end())
        {
            if ((*tIt)->GetDataType() == pDataType)
            {
                //### reset statistic
                if (tCount == pIndex)
                {
                    LOG(LOG_VERBOSE, "Going to reset statistic for %s", (*tIt)->GetStreamName().c_str());
                    (*tIt)->ResetPacketStatistic();
                    break;
                }
                tCount++;
            }

            //### next entry
            tIt++;
        }
    }

    SVC_PACKET_STATISTIC.ReleasePacketStatisticsAccess();
}

void OverviewDataStreamsWidget::SaveCompleteStatistic()
{
    QString tFileName = QFileDialog::getSaveFileName(this,
                                                     Homer::Gui::OverviewDataStreamsWidget::tr("Save packet statistic"),
                                                     QDir::homePath() + Homer::Gui::OverviewDataStreamsWidget::tr("/PacketStatistic.csv"),
                                                     Homer::Gui::OverviewDataStreamsWidget::tr("Comma-Separated Values File") + " (*.csv)",
                                                     NULL,
                                                     CONF_NATIVE_DIALOGS);

    if (tFileName.isEmpty())
        return;

    // write history to file
    QFile tFile(tFileName);
    if (!tFile.open(QIODevice::ReadWrite))
    {
        ShowError(Homer::Gui::OverviewDataStreamsWidget::tr("Unable to open file"), Homer::Gui::OverviewDataStreamsWidget::tr("The selected output file can't be opened"));
        return;
    }

    //#####################################################
    //### write header to csv
    //#####################################################
    QString tHeader = "Type,MinSize,MaxSize,AvgSize,Size,Packets,LostPackets,Direction,Rate,MomRate\n";
    if (!tFile.write(tHeader.toStdString().c_str(), tHeader.size()))
        return;

    //#####################################################
    //### write one entry per line
    //#####################################################
    QString tLine;
    PacketStatistics::iterator tIt;
    PacketStatistics tStatList = SVC_PACKET_STATISTIC.GetPacketStatisticsAccess();

    if (tStatList.size() > 0)
    {
        tIt = tStatList.begin();

        while (tIt != tStatList.end())
        {
            PacketStatisticDescriptor tStatValues = (*tIt)->GetPacketStatistic();

            //#######################
            //### calculate line
            //#######################
            tLine = QString((*tIt)->GetStreamName().c_str()) + ",";
            tLine += QString("%1,").arg(tStatValues.MinPacketSize);
            tLine += QString("%1,").arg(tStatValues.MaxPacketSize);
            tLine += QString("%1,").arg(tStatValues.AvgPacketSize);
            tLine += QString("%1,").arg(tStatValues.ByteCount);
            tLine += QString("%1,").arg(tStatValues.PacketCount);
            tLine += QString("%1,").arg(tStatValues.LostPacketCount);
            if (tStatValues.Outgoing)
                tLine += "outgoing,";
            else
                tLine += "incoming,";
            tLine += QString("%1,").arg(tStatValues.AvgDataRate);
            tLine += QString("%1").arg(tStatValues.MomentAvgDataRate);
            tLine += "\n";

            //#######################
            //### write to file
            //#######################
            if (!tFile.write(tLine.toStdString().c_str(), tLine.size()))
                return;

            //#######################
            //### next entry
            //#######################
            tIt++;
        }
    }

    SVC_PACKET_STATISTIC.ReleasePacketStatisticsAccess();

    tFile.close();
}

void OverviewDataStreamsWidget::FillCellText(QTableWidget *pTable, int pRow, int pCol, QString pText)
{
    if (pTable->item(pRow, pCol) != NULL)
        pTable->item(pRow, pCol)->setText(pText);
    else
    {
        QTableWidgetItem *tItem =  new QTableWidgetItem(pText);
        tItem->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        pTable->setItem(pRow, pCol, tItem);
    }
}

void OverviewDataStreamsWidget::FillRow(QTableWidget *pTable, int pRow, PacketStatistic *pStats)
{
	if (pStats == NULL)
		return;

	PacketStatisticDescriptor tStatValues = pStats->GetPacketStatistic();

	if (pRow > pTable->rowCount() - 1)
        pTable->insertRow(pTable->rowCount());

    if (pTable->item(pRow, 0) != NULL)
        pTable->item(pRow, 0)->setText(QString(pStats->GetStreamName().c_str()));
    else
        pTable->setItem(pRow, 0, new QTableWidgetItem(QString(pStats->GetStreamName().c_str())));
    pTable->item(pRow, 0)->setBackground(QBrush(QColor(Qt::lightGray)));

    FillCellText(pTable, pRow, 1, Int2ByteExpression(tStatValues.MinPacketSize) + " bytes");
    FillCellText(pTable, pRow, 2, Int2ByteExpression(tStatValues.MaxPacketSize) + " bytes");
    FillCellText(pTable, pRow, 3, Int2ByteExpression(tStatValues.AvgPacketSize) + " bytes");
    FillCellText(pTable, pRow, 4, Int2ByteExpression(tStatValues.ByteCount) + " bytes");
    FillCellText(pTable, pRow, 5, Int2ByteExpression(tStatValues.PacketCount));
    FillCellText(pTable, pRow, 6, Int2ByteExpression((int64_t)tStatValues.LostPacketCount));
    FillCellText(pTable, pRow, 8, QString(pStats->GetTransportTypeStr().c_str()));
    pTable->item(pRow, 8)->setTextAlignment(Qt::AlignCenter|Qt::AlignVCenter);
    FillCellText(pTable, pRow, 9, QString(pStats->GetNetworkTypeStr().c_str()));
    pTable->item(pRow, 9)->setTextAlignment(Qt::AlignCenter|Qt::AlignVCenter);
    FillCellText(pTable, pRow, 10, Int2ByteExpression(tStatValues.MomentAvgDataRate) + " bytes/s");
    FillCellText(pTable, pRow, 11, Int2ByteExpression(tStatValues.AvgDataRate) + " bytes/s");
    FillCellText(pTable, pRow, 12, Int2ByteExpression(pRow));

	if (pTable->item(pRow, 7) != NULL)
        if (tStatValues.Outgoing)
            pTable->item(pRow, 7)->setText(QString("outgoing"));
        else
            pTable->item(pRow, 7)->setText(QString("incoming"));
    else if (tStatValues.Outgoing)
        pTable->setItem(pRow, 7, new QTableWidgetItem(QString("outgoing")));
    else
        pTable->setItem(pRow, 7, new QTableWidgetItem(QString("incoming")));
    pTable->item(pRow, 7)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QIcon tIcon11;
    if (tStatValues.Outgoing)
        tIcon11.addPixmap(QPixmap(":/images/32_32/ArrowRight.png"), QIcon::Normal, QIcon::Off);
    else
        tIcon11.addPixmap(QPixmap(":/images/32_32/ArrowLeft.png"), QIcon::Normal, QIcon::Off);
    pTable->item(pRow, 7)->setIcon(tIcon11);

}

void OverviewDataStreamsWidget::UpdateView()
{
	PacketStatistics::iterator tIt;
    int tRowAudio = 0, tRowVideo = 0;
    int tASelectedRow = -1, tVSelectedRow = -1;

    // save old widget state
    int tAudioOldVertSliderPosition = mTwAudio->verticalScrollBar()->sliderPosition();
    int tAudioOldHorizSliderPosition = mTwAudio->horizontalScrollBar()->sliderPosition();
    bool tAudioWasVertMaximized = (tAudioOldVertSliderPosition == mTwAudio->verticalScrollBar()->maximum());
    if (mTwAudio->selectionModel()->currentIndex().isValid())
        tASelectedRow = mTwAudio->selectionModel()->currentIndex().row();
    int tVideoOldVertSliderPosition = mTwVideo->verticalScrollBar()->sliderPosition();
    int tVideoOldHorizSliderPosition = mTwVideo->horizontalScrollBar()->sliderPosition();
    bool tVideoWasVertMaximized = (tVideoOldVertSliderPosition == mTwVideo->verticalScrollBar()->maximum());
    if (mTwVideo->selectionModel()->currentIndex().isValid())
        tVSelectedRow = mTwVideo->selectionModel()->currentIndex().row();

    // update widget content
    PacketStatistics tStatList = SVC_PACKET_STATISTIC.GetPacketStatisticsAccess();
    if (tStatList.size() > 0)
    {
        tIt = tStatList.begin();

        while (tIt != tStatList.end())
        {
        	switch((*tIt)->GetDataType())
        	{
				case DATA_TYPE_VIDEO:
                    FillRow(mTwVideo, tRowVideo++, *tIt);
					break;
				case DATA_TYPE_AUDIO:
                    FillRow(mTwAudio, tRowAudio++, *tIt);
					break;
        	}
            tIt++;
        }
    }
    SVC_PACKET_STATISTIC.ReleasePacketStatisticsAccess();

    for (int i = mTwAudio->rowCount(); i > tRowAudio; i--)
        mTwAudio->removeRow(i);
    for (int i = mTwVideo->rowCount(); i > tRowVideo; i--)
        mTwVideo->removeRow(i);

    // restore old widget state
    mTwAudio->setRowCount(tRowAudio);
    if (tASelectedRow != -1)
        mTwAudio->selectRow(tASelectedRow);
    if (tAudioWasVertMaximized)
        mTwAudio->verticalScrollBar()->setSliderPosition(mTwAudio->verticalScrollBar()->maximum());
    else
        mTwAudio->verticalScrollBar()->setSliderPosition(tAudioOldVertSliderPosition);
    mTwAudio->horizontalScrollBar()->setSliderPosition(tAudioOldHorizSliderPosition);
    mTwVideo->setRowCount(tRowVideo);
    if (tVSelectedRow != -1)
        mTwVideo->selectRow(tVSelectedRow);
    if (tVideoWasVertMaximized)
        mTwVideo->verticalScrollBar()->setSliderPosition(mTwVideo->verticalScrollBar()->maximum());
    else
        mTwVideo->verticalScrollBar()->setSliderPosition(tVideoOldVertSliderPosition);
    mTwVideo->horizontalScrollBar()->setSliderPosition(tVideoOldHorizSliderPosition);
}

void OverviewDataStreamsWidget::timerEvent(QTimerEvent *pEvent)
{
    #ifdef DEBUG_TIMING
        LOG(LOG_VERBOSE, "New timer event");
    #endif
   if (pEvent->timerId() == mTimerId)
        UpdateView();
}

///////////////////////////////////////////////////////////////////////////////

}} //namespace
