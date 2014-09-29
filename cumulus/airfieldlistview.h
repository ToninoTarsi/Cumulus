/***********************************************************************
**
**   airfieldlistview.h
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2002      by André Somers
**                   2008-2014 by Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
***********************************************************************/

/**
 * \class AirfieldListView
 *
 * \author André Somers, Axel Pauli
 *
 * \brief This widget provides a list of waypoints and a means to select one.
 *
 * \date 2002-2014
 *
 */

#ifndef AIRFIELD_LIST_VIEW_H
#define AIRFIELD_LIST_VIEW_H

#include <QMetaObject>
#include <QPushButton>
#include <QBoxLayout>
#include <QMainWindow>
#include <QVector>

#include "airfieldlistwidget.h"
#include "waypoint.h"
#include "mapcontents.h"

class AirfieldListView : public QWidget
{
  Q_OBJECT

private:
  /**
   * That macro forbids the copy constructor and the assignment operator.
   */
  Q_DISABLE_COPY( AirfieldListView )

public:

  AirfieldListView( QVector<enum MapContents::MapContentsListID> &itemList,
                    QWidget *parent=0);

  virtual ~AirfieldListView();

  AirfieldListWidget* listWidget()
    {
      return listw;
    };

  Waypoint* getCurrentEntry()
    {
      return listw->getCurrentWaypoint();
    };

  /**
   * \return The top level item count of the tree list.
   */
  int topLevelItemCount()
  {
    return listw->topLevelItemCount();
  };

private:

  AirfieldListWidget* listw;
  QBoxLayout *buttonrow;
  QPushButton *cmdSelect;
  QPushButton *cmdHome;

protected:

  void showEvent( QShowEvent *event );

public slots:
  /**
   * This signal is called to indicate that a selection has been made.
   */
  void slot_Select();
  /**
   * This slot is called if the info button has been clicked, or the user pressed 'i'
   */
  void slot_Info();
  /**
   * Called when the list view should be closed without selection
   */
  void slot_Close ();

  /**
   * Called to set a point as home site
   */
  void slot_Home();

  void slot_Selected();

  /**
   * Called to reload the airfield item list
   */
  void slot_reloadList()
  {
    listw->refillItemList();
  };

signals:
  /**
   * This signal is emitted if a new waypoint is selected.
   */
  void newWaypoint(Waypoint*, bool);

  /**
   * This signal is send if the selection is done, and the screen can be closed.
   */
  void done();

  /**
   * Emitted if the user clicks the Info button.
   */
  void info(Waypoint*);

  /**
   * Emitted if a new home position is selected
   */
  void newHomePosition(const QPoint&);

  /**
   * Emitted to move the map to the new home position
   */
  void gotoHomePosition();

private:

  /** that shall store a home position change */
  bool homeChanged;
};

#endif
