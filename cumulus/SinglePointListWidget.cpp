/***********************************************************************
**
**   SinglePointListWidget.cpp
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2014-2015 by Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
***********************************************************************/

#include "SinglePointListWidget.h"

#ifndef QT_5
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "generalconfig.h"
#include "mapconfig.h"
#include "calculator.h"

extern MapContents *_globalMapContents;
extern MapConfig   *_globalMapConfig;

SinglePointListWidget::SinglePointListWidget( QVector<enum MapContents::ListID> &itemList,
					      QWidget *parent,
					      bool showMovePage ) :
  ListWidgetParent( parent, showMovePage ),
  m_itemList(itemList)
{
  setObjectName("SinglePointListWidget");
  list->setObjectName("SinglePointTreeWidget");

  QTreeWidgetItem *headerItem = list->headerItem();
  headerItem->setText( 3, tr("Comment") );
}

SinglePointListWidget::~SinglePointListWidget()
{
}

/** Clears and refills the single point item list. */
void SinglePointListWidget::fillItemList()
{
  // call base class
  ListWidgetParent::fillItemList();

  list->setUpdatesEnabled(false);
  list->clear();

  configRowHeight();

  for ( int item = 0; item < m_itemList.size(); item++ )
    {
      int nr = _globalMapContents->getListLength( m_itemList.at(item) );

      for (int i = 0; i < nr; i++ )
        {
          SinglePoint* site = dynamic_cast<SinglePoint *> (_globalMapContents->getElement( m_itemList.at(item), i ));

          if( site == 0 )
            {
              continue;
            }

          filter->addListItem( new SinglePointItem(site) );
        }
    }

  // sorting is done in filter->reset()
  resizeListColumns();

  filter->reset();
  list->setUpdatesEnabled(true);
}

/** Returns a pointer to the currently highlighted airfield. */
Waypoint* SinglePointListWidget::getCurrentWaypoint()
{
  // qDebug("SinglePointListWidget::getCurrentWaypoint()");
  QTreeWidgetItem* li = list->currentItem();

  if( li == 0 )
    {
      return 0;
    }

  SinglePointItem* item = dynamic_cast<SinglePointItem *> (li);

  // May be null if the cast failed.
  if( item == static_cast<SinglePointItem *> (0) )
    {
      return static_cast<Waypoint *> (0);
    }

  SinglePoint* site = item->m_singlePoint;

  m_wp.name = site->getWPName();
  m_wp.wgsPoint = site->getWGSPosition();
  m_wp.elevation = site->getElevation();
  m_wp.projPoint = site->getPosition();
  m_wp.description = site->getName();
  m_wp.type = site->getTypeID();
  m_wp.elevation = site->getElevation();
  m_wp.comment = site->getComment();
  m_wp.country = site->getCountry();

  return &m_wp;
}

SinglePointListWidget::SinglePointItem::SinglePointItem(SinglePoint* site) :
  QTreeWidgetItem(), m_singlePoint(site)
{
  setText(0, site->getWPName());
  setText(1, site->getName());
  setText(2, site->getCountry());
  setTextAlignment(2, Qt::AlignCenter);
  setText(3, site->getComment());

  // set type icon
  QPixmap pm = _globalMapConfig->getPixmap(site->getTypeID(), false);
  setIcon( 0, QIcon( pm) );
}
