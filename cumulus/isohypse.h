/***********************************************************************
 **
 **   isohypse.h
 **
 **   This file is part of Cumulus.
 **
 ************************************************************************
 **
 **   Copyright (c):  2000 by Heiner Lamprecht, Florian Ehinge
 **                   2007 Axel Pauli
 **
 **   This file is distributed under the terms of the General Public
 **   Licence. See the file COPYING for more information.
 **
 **   $Id$
 **
 ***********************************************************************/

#ifndef ISOHYPSE_H
#define ISOHYPSE_H

#include <QRect>
#include <QRegion>

#include "lineelement.h"

/**
 * This class is used for isohypses.
 *
 * @author Heiner Lamprecht, Florian Ehinger
 */
class Isohypse : public LineElement
  {
  public:
    /**
     * Creates a new isohypse.
     *
     * @param  pG  The polygon containing the position points.
     * @param  elev  The elevation
     * @param  isValles "true", if the area is a valley
     */
    Isohypse(QPolygon pG, unsigned int elev, bool isValley, unsigned int secID);

    /**
     * Destructor
     */
    virtual ~Isohypse();

    /**
     * Draws the iso region into the given painter.
     *
     * @param  targetP  The painter to draw the element into.
     */
    QRegion* drawRegion( QPainter* targetP, const QRect &viewRect,
                         bool really_draw, bool isolines=false );

    /**
     * @return the elevation of the line
     */
    int getElevation() const
      {
        return elevation;
      };


  private:
    /**
     * The elevation
     */
    int elevation;

    /**
     * "true", if element is a valley, that is lower than the
     * surrounding, underlying terrain.
     */
    bool valley;

  };

#endif
