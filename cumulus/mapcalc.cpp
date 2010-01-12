/***********************************************************************
**
**   mapcalc.cpp
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  1999, 2000 by Heiner Lamprecht, Florian Ehinger
**                   2008-2010  by Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#include <stdlib.h>
#include <cmath>

#include <QtGlobal>

#include "mapcalc.h"
#include "mapmatrix.h"

static const double rad = M_PI / 108000000.0; // Pi / (180 degrees * 600000 KFlog degrees)
static const double pi_180 = M_PI / 108000000.0;

/**
 * Calculates the distance between two given points in km according to Pythagoras.
 * For longer distances the result is often greater as calculated by great circle.
 * Not more used in Cumulus.
 */
double distP(double lat1, double lon1, double lat2, double lon2)
{
  double dlat = lat1 - lat2;
  double dlon = lon1 - lon2;

  // lat is used to calculate the earth-radius. We use the average here.
  // Otherwise, the result would depend on the order of the parameters.
  double lat = ( lat1 + lat2 ) / 2.0;

  //  double dist = RADIUS * sqrt( ( pi_180 * dlat * pi_180 * dlat )
  //    + ( pi_180 * cos( pi_180 * lat ) * dlon *
  //        pi_180 * cos( pi_180 * lat ) * dlon ) );
  // hypot (x, y) == sqrt (x * x + y * y);

  // Distance calculation according to Pythagoras
  double dist = RADIUS * hypot (pi_180 * dlat, pi_180 * cos( pi_180 * lat ) * dlon);

  return dist / 1000.0;
}

/**
 *  Distance calculation according to great circle. Unfit for very short
 *  distances due to rounding errors but required for longer distances according
 *  to FAI Code Sportif, Annex C. It is used as default in Cumulus.
 *  http://sis-at.streckenflug.at/2009/pdf/cs_annex_c.pdf
 */
double dist(double lat1, double lon1, double lat2, double lon2)
{
  // See here for formula: http://williams.best.vwh.net/avform.htm#Dist and
  // http://freenet-homepage.de/streckenflug/optigc.html
  // s = 6371km * acos( sin(lat1) * sin(lat2) + cos(lat1) * cos(lat2) * cos(lon1-lon2) )
  double dlon = lon1-lon2;

  double arc = acos( sin(lat1*rad) * sin(lat2*rad) + cos(lat1*rad) * cos(lat2*rad) * cos(dlon*rad) );

  // distance in Km
  double dist = arc * RADIUS / 1000.;

  return dist;
}

/**
 * Distance calculation according to great circle. A mathematically equivalent formula,
 * which is less subject to rounding error for short distances.
 */
double distC1(double lat1, double lon1, double lat2, double lon2)
{
  // See here for formula: http://williams.best.vwh.net/avform.htm#Dist
  // d = 6371km * 2 * asin( sqrt( sin((lat1-lat2)/2) ^2 + cos(lat1) * cos(lat2) * sin((lon1-lon2)/2) ^2 ) )
  double dlon = (lon1-lon2) * rad / 2;
  double dlat = (lat1-lat2) * rad / 2;

  double sinLatd = sin(dlat);
  sinLatd = sinLatd * sinLatd;

  double sinLond = sin(dlon);
  sinLond = sinLond * sinLond;

  double arc = 2 * asin( sqrt( sinLatd + cos(lat1*rad) * cos(lat2*rad) * sinLond ) );

  // distance in Km
  double dist = arc * RADIUS / 1000.;

  return dist;
}

double dist(QPoint* p1, QPoint* p2)
{
    return ( dist( double(p1->x()), double(p1->y()),
                   double(p2->x()), double(p2->y()) ));
}


double dist(wayPoint* wp1, wayPoint* wp2)
{
    return ( dist( wp1->origP.lat(), wp1->origP.lon(),
                   wp2->origP.lat(), wp2->origP.lon() ) );
}


double dist(wayPoint* wp, FlightPoint* fp)
{
    return ( dist( wp->origP.lat(), wp->origP.lon(),
                   fp->origP.lat(), fp->origP.lon() ) );
}


double dist(FlightPoint* fp1,  FlightPoint* fp2)
{
    return ( dist( fp1->origP.lat(), fp1->origP.lon(),
                   fp2->origP.lat(), fp2->origP.lon() ) );
}


QString printTime(int time, bool isZero, bool isSecond)
{
    QString hour, min, sec;

    int hh = time / 3600;
    int mm = (time - (hh * 3600)) / 60;
    int ss = time - (hh * 3600) - mm * 60;

    if( isZero )
        hour.sprintf("%0d", hh);
    else
        hour.sprintf("%d", hh);

        min.sprintf("%02d", mm);

        sec.sprintf("%02d", ss);

    if(isSecond)
        return (hour + ":" + min + ":" + sec);

    return ( hour + ":" + min );
}


float getSpeed(FlightPoint p)
{
    return (float)p.dS / (float)p.dT * 3.6;
}


float getVario(FlightPoint p)
{
    return (float)p.dH / (float)p.dT;
}


float getBearing(FlightPoint p1, FlightPoint p2)
{
    return (float)polar( ( p2.projP.x() - p1.projP.x() ),
                         ( p2.projP.y() - p1.projP.y() ) );
}


float getBearing(QPoint p1, QPoint p2)
{
    return (float) getBearingWgs(p1, p2);
}


// @AP: Note the bearing is computed with coordinates mapped to the
// current selected projection.
double getBearing2(QPoint p1, QPoint p2)
{
    extern MapMatrix * _globalMapMatrix;
    double pp1x, pp1y, pp2x, pp2y = 0.0;
    double angle=0.0;
    double dx=0.0;
    double dy=0.0;

    // qDebug("p1=(%d,%d) p2=(%d,%d)", p1.x(),p1.y(),p2.x(),p2.y());

    _globalMapMatrix->wgsToMap(p1.x(), p1.y(), pp1x, pp1y);
    _globalMapMatrix->wgsToMap(p2.x(), p2.y(), pp2x, pp2y);

    // qDebug("BearingWGS: pp1=(%f,%f) pp2=(%f,%f)", pp1x, pp1y, pp2x, pp2y);

    dx = pp2x-pp1x;
    dy = pp2y-pp1y;

    if (dy>=-0.001 && dy<=0.001) {
        if (dx < 0.0)
            return (1.5 * M_PI);
        else
            return (M_PI_2);
    }

    angle=atan(dx/-dy);
    if (dy>0.0)
        angle+= M_PI;
    if (angle<0)
        angle+=(PI2);
    if (angle>(2* M_PI ))
        angle-=(PI2);

    return angle;
}


/**
   Calculate the bearing from point p1 to point p2 from WGS84
   coordinates to avoid distortions caused by projection to the map.
*/
double getBearingWgs( QPoint p1, QPoint p2 )
{
    // Arcus computing constant for kflog corordinates. PI is devided by
    // 180 degrees multiplied with 600.000 because one degree in kflog
    // is multiplied with this resolution factor.
    const double pi_180 = M_PI / 108000000.0;

    // qDebug( "x1=%d y1=%d, x2=%d y2=%d",  p1.x(), p1.y(), p2.x(), p2.y() );

    int dx = p2.x() - p1.x(); // latitude
    int dy = p2.y() - p1.y(); // longitude

    // compute latitude distance in meters
    double latDist = dx * MILE_kfl / 10000.; // b

    // compute latitude average
    double latAv = ( ( p2.x() + p1.x() ) / 2.0);

    // compute longitude distance in meters
    double lonDist = dy * cos( pi_180 * latAv ) * MILE_kfl / 10000.; // a

    // compute angle
    double angle = asin( fabs(lonDist) / hypot( latDist, lonDist ) );

    // double angleOri = angle;

    // assign computed angle to the right quadrant
    if( dx >= 0 && dy < 0 ) {
        angle = (2 * M_PI) - angle;
    } else if( dx <=0 && dy <= 0 ) {
        angle =  M_PI + angle;
    } else if( dx < 0 && dy >= 0 ) {
        angle = M_PI - angle;
    }

    //qDebug( "dx=%d, dy=%d - AngleRadOri=%f, AngleGradOri=%f - AngleRadCorr=%f, AngleGradCorr=%f",
    //  dx, dy, angleOri, angleOri * 180/M_PI, angle, angle * 180/M_PI);

    return angle;
}


double polar(double y, double x)
{
    double angle = 0.0;
    //
    //  dX = 0 ???
    //
    if(x >= -0.001 && x <= 0.001) {
        if(y < 0.0)
            return ( 1.5 * M_PI );
        else
            return ( M_PI_2 );
    }

    // Punkt liegt auf der neg. X-Achse
    if(x < 0.0)
        angle = atan( y / x ) + M_PI;
    else
        angle = atan( y / x );

    // Neg. value not allowed.
    if(angle < 0.0)
        angle = PI2 + angle;

    if(angle > (PI2))
        angle = angle - (PI2);

    return angle;
}


/**
 * Calculates the direction of the vector pointing to the outside
 * of the area spanned by the two vectors.
 */
double outsideVector(QPoint center, QPoint p1, QPoint p2)
{
    double v1=getBearing(center, p1);
    double v2=getBearing(center, p2);

    double res1=(v1+v2)/2;
    double res2=res1+M_PI;

    // qDebug("outsideVector: v1=%f, v2=%f, res1=%f, res2=%f",v1,v2,res1,res2);
    res1=normalize(res1);
    res2=normalize(res2);

    if (res1 - qMin(v1, v2) < M_PI_2)
      {
        return res1;
      }
    else
      {
        return res2;
      }
}


double normalize(double angle)
{
    //we needed to use a modulo for the integer version. We should
    //perhaps use something similar here?
    if (angle<0)
        return normalize(angle+PI2);
    if (angle>=PI2)
        return normalize(angle-PI2);
    return angle;
}


int normalize(int angle)
{
    if (angle >= 360)
        return angle%360;
    if (angle < -360)
        return normalize(angle%360);
    if (angle<0)
        return normalize(angle+360);
/*    if (angle>=360)
    return normalize(angle-360);*/
    return angle;
}


int angleDiff(int ang1, int ang2)
{
    int a1=normalize(ang1);
    int a2=normalize(ang2);
    int a=a2-a1;
    if (a>180)
        return(a-360);
    if (a<-180)
        return(a+360);
    return a;
}


double angleDiff(double ang1, double ang2)
{
    double a1=normalize(ang1);
    double a2=normalize(ang2);
    double a=a2-a1;
    if (a>M_PI)
        return(a-PI2);
    if (a<-M_PI)
        return(a+PI2);
    return a;
}


/**
 * Calculates a (crude) bounding box that contains the circle of radius @arg r
 * around point @arg center. @arg r is given in kilometers.
 */
QRect areaBox(QPoint center, double r)
{
    const double pi_180 = M_PI / 108000000.0;
    int delta_lat = (int) rint(250.0 * r/RADIUS_kfl);
    int delta_lon = (int) rint(r*250.0 / (RADIUS_kfl *cos (pi_180 * center.x())));

    //  qDebug("delta_lat=%d, delta_lon=%d, reach=%f, center=(%d, %d)",delta_lat, delta_lon,r,center.x(),center.y());
    //  return QRect(center.x()-delta_lat, center.y()-delta_lon,
    //                      center.x()+delta_lat, center.y()+delta_lon);
    return QRect(center.x()-delta_lat, center.y()-delta_lon,
                 2*delta_lat, 2*delta_lon);
}

/**
 * Calculates the bounding box of the given tile number in KFLog coordinates.
 * The returned rectangle used the x-axis as longitude and the y-axis as latitude.
 */
QRect getTileBox(const ushort tileNo)
{
  if( tileNo > (180*90) )
    {
      qWarning("Tile %d is out of range", tileNo);
      return QRect();
    }

  // Positive result means N, negative result means S
  int lat = 90 - ((tileNo / 180) * 2);

  // Positive result means E, negative result means W
  int lon = ((tileNo % 180) * 2) - 180;

  // Tile bounding rectangle starting at upper left corner with:
  // X: longitude until longitude + 2 degrees
  // Y: latitude  until latitude - 2 degrees
  QRect rect( lon*600000, lat*600000, 2*600000, -2*600000 );

/*
  qDebug("Tile=%d, Lat=%d, Lon=%d, X=%d, Y=%d, W=%d, H=%d",
          tileNo, lat, lon, rect.x(), rect.y(),
          (rect.x()+rect.width())/600000,
          (rect.y()+rect.height())/600000 );
*/

 return rect;
}

/**
 * Calculates ground speed, wca and true course via the wind triangle.
 */
void windTriangle( const double trueHeading,
                   const double trueAirSpeed,
                   const double windDirection,
                   const double windSpeed,
                   double &groundSpeed,
                   double &wca,
                   double &trueCourse )
{
  double d1, d2, d3, d4;

  d1 = trueHeading - windDirection;

  if( d1 < 0.0 )
    {
      d2 = -(d1 + 180.0);
    }
  else
    {
      d2 = d1 - 180.0;
    }

  d3 = (sin(d2 * M_PI / 180.0) * windSpeed) / trueAirSpeed;

  wca = asin( d3 ) * 180.0 / M_PI;
  d4 = 180 - d2 - wca;

  groundSpeed = (sin( d4 * M_PI / 180.0) * windSpeed) / ( sin(wca * M_PI / 180.0));
  trueCourse = trueHeading - wca;

  if( trueCourse < 0.0 )
    {
      trueCourse += 360.0;
    }
  else if( trueCourse >= 360.0 )
    {
      trueCourse -= 360.0;
    }

  // round results to integer
  wca = rint(wca);
  trueCourse = rint(trueCourse);
  groundSpeed = rint(groundSpeed);
}
