/***********************************************************************
 **
 **   openairparser.cpp
 **
 **   This file is part of Cumulus.
 **
 ************************************************************************
 **
 **   Copyright (c):  2005 by Andr� Somers, 2008 Axel Pauli
 **
 **   This file is distributed under the terms of the General Public
 **   Licence. See the file COPYING for more information.
 **
 **   $Id$
 **
 ***********************************************************************/

//standard libs includes
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

//Qt includes
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QStringList>
#include <QTextStream>

//project includes
#include "resource.h"
#include "airspace.h"
#include "openairparser.h"
#include "mapcalc.h"
#include "mapmatrix.h"
#include "mapcontents.h"
#include "projectionbase.h"
#include "filetools.h"

// All is prepared for additional calculation, storage and
// reconstruction of a bounding box. Be free to switch on/off it via
// conditional define.

#undef BOUNDING_BOX
// #define BOUNDING_BOX 1

// type definition for compiled airspace files
#define FILE_TYPE_AIRSPACE_C 0x61

// version used for files created from OpenAir data
#define FILE_VERSION_AIRSPACE_C 200

extern MapContents*  _globalMapContents;
extern MapMatrix*    _globalMapMatrix;

OpenAirParser::OpenAirParser()
{
  _doCompile = false;
  _boundingBox = (QRect *) 0;
  _bufData = (QByteArray *) 0;
  _outbuf = (QDataStream *) 0;
  h_projection = (ProjectionBase *) 0;

  initializeBaseMapping();
}


OpenAirParser::~OpenAirParser()
{
  if( h_projection ) {
    delete h_projection;
  }
}

/**
 * Searchs on default places for openair files. That can be source
 * files or compiled versions of them.
 *
 * @returns number of successfully loaded files
 * @param list the list of Airspace objects the objects in this
 *   file should be added to.
 */

uint OpenAirParser::load( Q3PtrList<Airspace>& list )
{
  QTime t;
  t.start();
  uint loadCounter = 0; // number of successfully loaded files

  QStringList preselect;
  MapContents::addDir(preselect, MapContents::mapDir1 + "/airspaces", "*.txt");
  MapContents::addDir(preselect, MapContents::mapDir1 + "/airspaces", "*.txc");
  MapContents::addDir(preselect, MapContents::mapDir2 + "/airspaces", "*.txt");
  MapContents::addDir(preselect, MapContents::mapDir2 + "/airspaces", "*.txc");
  MapContents::addDir(preselect, MapContents::mapDir3 + "/airspaces", "*.txt");
  MapContents::addDir(preselect, MapContents::mapDir3 + "/airspaces", "*.txc");

  if(preselect.count() == 0) {
    qWarning( "OpenAirParser: No Open Air files could be found in the map directories" );
    return loadCounter;
  }

  // source files follows compiled files
  preselect.sort();

  while ( ! preselect.isEmpty() ) {
    QString txtName, txcName;
    _doCompile = false;

    if ( preselect.first().contains( QString(".txt") ) ) {
      // there can't be the same name txc after this txt
      // parse found txt file
      txtName = preselect.first();
      _doCompile = true;

      if( parse( txtName, list ) )
        loadCounter++;

      preselect.remove( preselect.begin() );
      continue;
    }

    // At first we found a binary file with the extension txc.
    txcName = preselect.first();

    // Now we have to check if there's to find a source file with
    // extension txt after the binary file
    txcName = preselect.first();
    preselect.remove( preselect.begin() );
    txtName = txcName;
    txtName.replace( txtName.length()-1, 1, QString("t") );

    if ( ! preselect.isEmpty() && txtName == preselect.first() ) {
      preselect.remove( preselect.begin() );
      // We found the related source file and will do some checks to
      // decide which type of file will be read in.

      // Lets check, if we can read the header of the compiled file
      if( ! setHeaderData( txcName ) ) {
        // Compiled file format is not the expected one, remove
        // wrong file and start a reparsing of source file.
        unlink( txcName.toLatin1().data() );
        _doCompile = true;

        if( parse( txtName, list ) )
          loadCounter++;

        preselect.remove( preselect.begin() );
        continue;
      }

      // Do a date-time check. If the source file is younger in its
      // modification time as the compiled file, a new compilation
      // must be forced.

      QFileInfo fi(txtName);
      QDateTime lastModTxt = fi.lastModified();

      if( h_creationDateTime < lastModTxt ) {
        // Modification date-time of source is younger as from
        // compiled file. Therefore we do start a reparsing of the
        // source file.
        unlink( txcName.toLatin1().data() );
        _doCompile = true;
        if( parse( txtName, list ) )
          loadCounter++;
        continue;
      }

      // Check date-time against the config file
      QString confName = fi.dirPath(TRUE) + "/" + fi.baseName() + "_mappings.conf";
      QFileInfo fiConf( confName );

      if( fiConf.exists() && fi.isReadable() &&
          h_creationDateTime < fiConf.lastModified() ) {
        // Conf file was modified, make a new compilation. It is not
        // deeper checked, what was modified due to the effort and
        // in the assumption that a config file will not be changed
        // every minute.
        unlink( txcName.toLatin1().data() );
        _doCompile = true;
        if( parse( txtName, list ) )
          loadCounter++;
        continue;
      }

      // Check, if the projection has been changed in the meantime
      ProjectionBase *currentProjection = _globalMapMatrix->getProjection();

      if( ! MapContents::compareProjections( h_projection, currentProjection ) ) {
        // Projection has changed in the meantime
        if(  h_projection ) {
          delete h_projection;
          h_projection = 0;
        }

        unlink( txcName.toLatin1().data() );
        _doCompile = true;
        if( parse( txtName, list ) )
          loadCounter++;
        continue;
      }
    }

    // we will read the compiled file, because a source file is not to
    // find after it or all checks were successfully passed
    if( readCompiledFile( txcName, list ) )
      loadCounter++;

  } // End of While

  qDebug("OpenAirParser: %d OpenAir file(s) loaded in %dms", loadCounter, t.elapsed());
  return loadCounter;
}


bool OpenAirParser::parse(const QString& path, Q3PtrList<Airspace>& list)
{
  QTime t;
  t.start();
  QFile source(path);

  if (!source.open(IO_ReadOnly)) {
    qWarning("OpenAirParser: Cannot open airspace file %s!", path.toLatin1().data());
    return false;
  }

  resetState();
  initializeStringMapping( path );

  if( _doCompile ) {
    // we open a buffer for temporary storage of extracted airspace
    // elements
    _bufData = new QByteArray();
    _buffer = new QBuffer();
    _outbuf = new QDataStream();

#ifdef BOUNDING_BOX
    _boundingBox = new QRect();
#endif

    _buffer->setBuffer( _bufData );
    _buffer->open(IO_ReadWrite);
    _outbuf->setDevice( _buffer );
  }

  QTextStream in(&source);
  //start parsing
  QString line=in.readLine();
  _lineNumber++;

  while (!line.isNull()) {
    // qDebug("reading line %d: '%s'", _lineNumber, line.toLatin1().data());
    line = line.simplified();

    if (line.startsWith("*") || line.startsWith("#")) {
      //comment, ignore
    } else if (line.isEmpty()) {
      //empty line, ignore
    } else {
      parseLine(line);
    }
    line=in.readLine();
    _lineNumber++;
  }

  if (_isCurrentAirspace)
    finishAirspace();

  for (uint i  = 0; i < _airlist.count(); i++) {
    list.append(_airlist.at(i));
  }

  QFileInfo fi( path );
  qDebug( "OpenAirParser: %d airspace objects read from file %s in %dms",
          _objCounter, fi.fileName().toLatin1().data(), t.elapsed() );

  source.close();

  // handle creation of a compiled file version
  if( _doCompile ) {
    // close opened buffer
    _buffer->close();

    if( _objCounter ) {
      // airspace objects were created during parsing, we will save
      // them in a txc file.
      QFile compFile;
      QDataStream out;

      // build the compiled file name with the extension .txc from
      // the source file name
      QString cfn = fi.dirPath( true ) + "/" + fi.baseName() + ".txc";

      compFile.setName( cfn );
      out.setDevice( &compFile );

      if( !compFile.open(IO_WriteOnly) ) {
        // Can't open output file, reset compile flag
        qWarning("OpenAirParser: Cannot open file %s!", cfn.toLatin1().data());
        _doCompile = false;
      } else {

        qDebug("OpenAirParser: writing file %s", cfn.toLatin1().data());

        // create compiled binary version
        out << quint32( KFLOG_FILE_MAGIC );
        out << qint8( FILE_TYPE_AIRSPACE_C );
        out << quint16( FILE_VERSION_AIRSPACE_C );
        out << QDateTime::currentDateTime();
        SaveProjection( out, _globalMapMatrix->getProjection() );

#ifdef BOUNDING_BOX

        out << *_boundingBox;
#endif
        // write data on airspaces from buffer
        out << *_bufData;
        compFile.close();
      }
    }

    // delete all dynamically allocated objects

#ifdef BOUNDING_BOX
    delete _boundingBox;
    _boundingBox = 0;
#endif

    delete _outbuf;
    _outbuf  = 0;
    delete _buffer;
    _buffer  = 0;
    delete _bufData;
    _bufData = 0;
  }

  return true;
}


void OpenAirParser::resetState()
{
  _airlist.clear();
  _direction = 1;
  _lineNumber = 0;
  _objCounter = 0;
  _isCurrentAirspace = false;
}


void OpenAirParser::parseLine(QString& line)
{
  if (line.startsWith("AC ")) {
    //type of record. This also indicates we're starting a new object
    if (_isCurrentAirspace)
      finishAirspace();
    newAirspace();
    parseType(line);
    return;
  }

  //the rest of the records don't make sence if we're not parsing an object
  int lat, lon;
  double radius;
  bool ok;

  if (!_isCurrentAirspace)
    return;

  if (line.startsWith("AN ")) {
    //name
    asName = line.mid(3);
    return;
  }

  if (line.startsWith("AH ")) {
    //airspace ceiling
    QString alt = line.mid(3);
    parseAltitude(alt, asUpperType, asUpper);
    return;
  }

  if (line.startsWith("AL ")) {
    //airspace floor
    QString alt = line.mid(3);
    parseAltitude(alt, asLowerType, asLower);
    return;
  }

  if (line.startsWith("DP ")) {
    //polygon coordinate
    QString coord = line.mid(3);
    parseCoordinate(coord, lat, lon);
    asPA.resize(asPA.count()+1);
    asPA.setPoint(asPA.count()-1,lat,lon);
    // qDebug( "addDP: lat=%d, lon=%d", lat, lon );
    return;
  }

  if (line.startsWith("DC ")) {
    //circle
    radius=line.mid(3).toDouble(&ok);
    if (ok) {
      addCircle(radius);
    }
    return;
  }

  if (line.startsWith("DA ")) {

    makeAngleArc(line.mid(3));
    return;
  }

  if (line.startsWith("DB ")) {
    makeCoordinateArc(line.mid(3));
    return;
  }

  if (line.startsWith("DY ")) {
    //airway
    return;
  }

  if (line.startsWith("V ")) {
    parseVariable(line.mid(2));
    return;
  }

  //ignored record types
  if (line.startsWith("AT ")) {
    //label placement, ignore
    return;
  }

  if (line.startsWith("TO ")) {
    //terrain open polygon, ignore
    return;
  }

  if (line.startsWith("TC ")) {
    //terrain closed polygon, ignore
    return;
  }

  if (line.startsWith("SP ")) {
    //pen definition, ignore
    return;
  }

  if (line.startsWith("SB ")) {
    //brush definition, ignore
    return;
  }
  //unknown record type
}


void OpenAirParser::newAirspace()
{
  asName = "(unnamed)";
  asType = BaseMapElement::NotSelected;
  //asTypeLetter = "";
  asPA.resize(0);
  asUpper = BaseMapElement::NotSet;
  asUpperType = BaseMapElement::NotSet;
  asLower = BaseMapElement::NotSet;
  asLowerType = BaseMapElement::NotSet;
  _isCurrentAirspace = true;
  _direction = 1; //must be reset according to specs
}


void OpenAirParser::newPA()
{
  asPA.resize(0);
}


void OpenAirParser::finishAirspace()
{
  extern MapMatrix * _globalMapMatrix;

  uint cnt=asPA.count();
  for (uint i=0; i<cnt; i++)
    asPA.setPoint(i, _globalMapMatrix->wgsToMap(asPA.point(i)));

  Airspace * a= new Airspace(asName,
                             asType,
                             asPA,
                             asUpper, asUpperType,
                             asLower, asLowerType);
  _airlist.append(a);
  _objCounter++;
  _isCurrentAirspace = false;
  // qDebug("finalized airspace %s. %d points in airspace", asName.toLatin1().data(), cnt);

  if( _doCompile ) {
    // we temporary save the airspace element in a memory buffer to
    // store it persistently later on in a binary file.
    ShortSave( *_outbuf, asName );
    *_outbuf << quint8( asType );
    *_outbuf << quint8( asLowerType );
    *_outbuf << qint16( asLower );
    *_outbuf << quint8( asUpperType );
    *_outbuf << qint16( asUpper );
    ShortSave( *_outbuf, asPA );

#ifdef BOUNDING_BOX

    if( !asPA.isEmpty() ) {
      *_boundingBox |= asPA.boundingRect();
    }
#endif

  }
}


void OpenAirParser::initializeBaseMapping()
{
  // create a mapping from a string representation of the supported
  // aispace types in Cumulus to their integer codes
  m_baseTypeMap.clear();

  m_baseTypeMap.insert("AirA", BaseMapElement::AirA);
  m_baseTypeMap.insert("AirB", BaseMapElement::AirB);
  m_baseTypeMap.insert("AirC", BaseMapElement::AirC);
  m_baseTypeMap.insert("AirD", BaseMapElement::AirD);
  m_baseTypeMap.insert("AirElow", BaseMapElement::AirElow);
  m_baseTypeMap.insert("AirEhigh", BaseMapElement::AirEhigh);
  m_baseTypeMap.insert("AirF", BaseMapElement::AirF);
  m_baseTypeMap.insert("ControlC", BaseMapElement::ControlC);
  m_baseTypeMap.insert("ControlD", BaseMapElement::ControlD);
  m_baseTypeMap.insert("Danger", BaseMapElement::Danger);
  m_baseTypeMap.insert("Restricted", BaseMapElement::Restricted);
  m_baseTypeMap.insert("Prohibited", BaseMapElement::Prohibited);
  m_baseTypeMap.insert("LowFlight", BaseMapElement::LowFlight);
  m_baseTypeMap.insert("Tmz", BaseMapElement::Tmz);
  m_baseTypeMap.insert("SuSector", BaseMapElement::SuSector);
}

void OpenAirParser::initializeStringMapping(const QString& mapFilePath)
{
  //fist, initialize the mapping QMap with the defaults
  m_stringTypeMap.clear();

  m_stringTypeMap.insert("A", "AirA");
  m_stringTypeMap.insert("B", "AirB");
  m_stringTypeMap.insert("C", "AirC");
  m_stringTypeMap.insert("D", "AirD");
  m_stringTypeMap.insert("E", "AirElow");
  m_stringTypeMap.insert("F", "AirF");
  m_stringTypeMap.insert("GP", "Restricted");
  m_stringTypeMap.insert("R", "Restricted");
  m_stringTypeMap.insert("P", "Prohibited");
  m_stringTypeMap.insert("TRA", "Restricted");
  m_stringTypeMap.insert("Q", "Danger");
  m_stringTypeMap.insert("CTR", "ControlC");
  m_stringTypeMap.insert("TMZ", "Tmz");
  m_stringTypeMap.insert("W", "AirEhigh");
  m_stringTypeMap.insert("GSEC", "SuSector");

  //then, check to see if we need to update this mapping
  //construct file name for mapping file
  QFileInfo fi(mapFilePath);

  QString path = fi.dirPath() + "/" + fi.baseName() + "_mappings.conf";
  fi.setFile(path);
  if (fi.exists() && fi.isFile() && fi.isReadable()) {
    QFile f(path);
    if (!f.open(IO_ReadOnly)) {
      qWarning("OpenAirParser: Cannot open airspace mapping file %s!", path.toLatin1().data());
      return;
    }

    QTextStream in(&f);
    qDebug("Parsing mapping file '%s'.", path.toLatin1().data());

    //start parsing
    QString line = in.readLine();

    while (!line.isNull()) {
      line = line.simplified();
      if (line.startsWith("*") || line.startsWith("#")) {
        //comment, ignore
      } else if (line.isEmpty()) {
        //empty line, ignore
      } else {
        int pos = line.find("=");
        if (pos>0 && pos < int(line.length())) {
          QString key = line.left(pos).simplified();
          QString value = line.mid(pos+1).simplified();
          qDebug("  added '%s' => '%s' to mappings", key.toLatin1().data(), value.toLatin1().data());
          m_stringTypeMap.replace(key, value);
        }
      }
      line=in.readLine();
    }
  }
}


void OpenAirParser::parseType(QString& line)
{
  line=line.mid(3);

  if (!m_stringTypeMap.contains(line)) {
    //no mapping from the found type to a Cumulus basetype was found
    qWarning("OpenAirParser: Line=%d Type, '%s' not mapped to basetype. Object not interpretted.", _lineNumber, line.toLatin1().data());
    _isCurrentAirspace = false; //stop accepting other lines in this object
    return;
  } else {
    QString stringType = m_stringTypeMap[line];
    if (!m_baseTypeMap.contains(stringType)) {
      //the indicated basetype is not a valid Cumulus basetype.
      qWarning("OpenAirParser:=Line %d, Type '%s' is not a valid basetype. Object not interpretted.", _lineNumber, stringType.toLatin1().data());
      _isCurrentAirspace = false; //stop accepting other lines in this object
      return;
    } else {
      //all seems to be right with the world!
      asType = m_baseTypeMap[stringType];
    }
  }
}


void OpenAirParser::parseAltitude(QString& line, BaseMapElement::elevationType& type, int& alt)
{
  bool convertFromMeters=false;
  QString input = line;
  QStringList elements;
  int len=0, pos=0;
  QString part;
  bool ok;
  int num=0;

  type = BaseMapElement::NotSet;
  alt=0;
  // qDebug("line %d: parsing altitude '%s'", _lineNumber, line.toLatin1().data());
  //fist, split the string in parsable elements
  //we start with the text parts
  QRegExp reg("[A-Za-z]+");

  while (line.length()>0) {
    pos = reg.indexIn(line, pos+len);
    len = reg.matchedLength();
    if (pos<0) {
      break;
    }
    elements.append(line.mid(pos, len));
    //qDebug("element: '%s'", line.mid(pos, len).toLatin1().data());
    //line=line.mid(len);
  }

  //now, get our number parts

  reg.setPattern("[0-9]+");
  pos=0;
  len=0;

  while (line.length()>0) {
    pos = reg.indexIn(line, pos+len);
    len = reg.matchedLength();

    if (pos<0) {
      break;
    }
    elements.append(line.mid(pos, len));
    //qDebug("element: '%s'", line.mid(pos, len).toLatin1().data());
    line=line.mid(len);
  }

  // now, try parsing piece by piece
  // print it out

  for ( QStringList::Iterator it = elements.begin(); it != elements.end(); ++it ) {
    part = (*it).toUpper();
    BaseMapElement::elevationType newType = BaseMapElement::NotSet;

    // first, try to interpret as elevation type
    if (part=="AMSL" || part=="MSL") {
      newType=BaseMapElement::MSL;
    }
    else if (part=="GND" || part=="SFC" || part=="ASFC") {
      newType=BaseMapElement::GND;
    }
    else if (part.startsWith("UNL")) {
      newType=BaseMapElement::UNLTD;
    }
    else if (part=="FL") {
      newType=BaseMapElement::FL;
    }
        
    if( type == BaseMapElement::NotSet && newType != BaseMapElement::NotSet ) {
      type = newType;
      continue;
    }

    if( type != BaseMapElement::NotSet && newType != BaseMapElement::NotSet ) {
      // @AP: Here we stept into a problem. We found a second
      // elevation type. That can be only a mistake in the data
      // and will be ignored.
      qWarning( "OpenAirParser: Line=%d, '%s' contains more than one elevation type. Only first one is taken",
                _lineNumber, input.toLatin1().data());
      continue;
    }

    //see if it is a way of setting units to meters
    if (part=="M") {
      convertFromMeters=true;
      continue;
    }

    //try to interpret as a number
    num = part.toInt(&ok);
    if (ok) {
      alt = num;
    }

    //ignore other parts
  }

  if (convertFromMeters)
    alt = (int) rint( alt/Distance::mFromFeet);

  // qDebug("Line %d: Returned altitude %d, type %d", _lineNumber, alt, int(type));
}


bool OpenAirParser::parseCoordinate(QString& line, int& lat, int& lon)
{
  bool result=true;
  line=line.toUpper();

  int pos=0;
  lat=0;
  lon=0;

  QRegExp reg("[NSEW]");
  pos = reg.indexIn(line, 0);
  if (pos==-1)
    return false;

  QString part1=line.left(pos+1);
  QString part2=line.mid(pos+1);
  result &= parseCoordinatePart(part1, lat, lon);
  result &= parseCoordinatePart(part2, lat, lon);

  return result;
}


bool OpenAirParser::parseCoordinatePart(QString& line, int& lat, int& lon)
{
  const double factor[4]= {
    600000, 10000, 166.666666667, 0
  };
  const double factor100[4]= {
    600000, 10000, 100.0, 0
  };
  bool decimal=false;
  int cur_factor=0;
  double part=0;
  bool ok=false, found=false;
  int value=0;
  int len=0, pos=0;
  // qDebug("line=%s", line.toLatin1().data() );

  QRegExp reg("[0-9]+");

  if (line.isEmpty()) {
    qWarning("Tried to parse empty coordinate part! Line %d", _lineNumber);
    return false;
  }

  while (cur_factor<3 && line.length()) {
    pos=reg.indexIn(line, pos+len);
    len = reg.matchedLength();

    if (pos==-1) {
      break;
    } else {
      part=line.mid(pos, len).toInt(&ok);
      if (ok) {
        if( decimal )   {
          value+=(int) rint(part * factor100[cur_factor]);
          // qDebug("part=%f add=%d v=%d  cf=%d %d", part, int(part * factor[cur_factor]), value, cur_factor, decimal );
        } else {
          value+=(int) rint(part * factor[cur_factor]);
          // qDebug("part=%f add=%d v=%d  cf=%d %d", part, int(part * factor[cur_factor]), value, cur_factor, decimal );
        }
        found=true;
      } else {
        break;
      }
    }
    if (line.mid(pos+len,1)==":") {
      len++;
      decimal = false;
      cur_factor++;
      continue;
    } else if (line.mid(pos+len,1)==".") {
      len++;
      cur_factor++;
      decimal = true;
      continue;
    }
  }

  if (!found)
    return false;

  // qDebug("%s value=%d", line.ascii(), value);

  if (line.find('N')>0) {
    lat=value;
    return true;
  }
  if (line.find('S')>0) {
    lat=-value;
    return true;
  }
  if (line.find('E')>0) {
    lon=value;
    return true;
  }
  if (line.find('W')>0) {
    lon=-value;
    return true;
  }
  return false;
}


bool OpenAirParser::parseCoordinate(QString& line, QPoint& coord)
{
  int lat=0, lon=0;
  bool result = parseCoordinate(line, lat, lon);
  coord.setX(lat);
  coord.setY(lon);
  return result;
}


bool OpenAirParser::parseVariable(QString line)
{
  QStringList arguments=QStringList::split('=', line);
  if (arguments.count()<2)
    return false;

  QString variable=arguments[0].simplified().toUpper();
  QString value=arguments[1].simplified();
  // qDebug("line %d: variable = '%s', value='%s'", _lineNumber, variable.toLatin1().data(), value.toLatin1().data());
  if (variable=="X") {
    //coordinate
    return parseCoordinate(value, _center);
  }

  if (variable=="D") {
    //direction
    if (value=="+") {
      _direction=+1;
    } else if (value=="-") {
      _direction=-1;
    } else {
      return false;
    }
    return true;
  }

  if (variable=="W") {
    //airway width
    bool ok;
    double result=value.toDouble(&ok);
    if (ok) {
      _awy_width = result;
      return true;
    }
    return false;
  }

  if (variable=="Z") {
    //zoom visiblity at zoom level; ignore
    return true;
  }

  return false;
}


// DA radius, angleStart, angleEnd
// radius in nm, center defined by using V X=...
bool OpenAirParser::makeAngleArc(QString line)
{
  //qDebug("OpenAirParser::makeAngleArc");
  bool ok;
  double radius, angle1, angle2;

  QStringList arguments = QStringList::split(',', line);
  if (arguments.count()<3)
    return false;

  radius=arguments[0].stripWhiteSpace().toDouble(&ok);

  if ( !ok ) {
    return false;
  }

  angle1=arguments[1].stripWhiteSpace().toDouble(&ok);

  if (!ok) {
    return false;
  }

  angle2=arguments[2].stripWhiteSpace().toDouble(&ok);

  if (!ok) {
    return false;
  }

  int lat = _center.x();
  int lon = _center.y();

  // grenzen 180 oder 90 beachten!
  double distLat = dist( lat, lon, lat + 10000, lon );
  double distLon = dist( lat, lon, lat, lon + 10000 );

  double kmr = radius * MILE_kfl / 1000.;
  //qDebug( "distLat=%f, distLon=%f, radius=%fkm", distLat, distLon, kmr );

  addArc( kmr/(distLat/10000.), kmr/(distLon/10000.), angle1, angle2 );
  return true;
}


/**
   Calculate the bearing from point p1 to point p2 from WGS84
   coordinates to avoid distortions caused by projection to the map.
*/
double OpenAirParser::bearing( QPoint& p1, QPoint& p2 )
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


/**
 * DB coordinate1, coordinate2
 * center defined by using V X=...
 */
bool OpenAirParser::makeCoordinateArc(QString line)
{
  // qDebug("OpenAirParser::makeCoordinateArc");
  double radius, angle1, angle2;

  //split of the coordinates, and check the number of arguments
  QStringList arguments = QStringList::split(',', line);
  if (arguments.count()<2)
    return false;

  QPoint coord1, coord2;

  //try to parse the coordinates
  if (!(parseCoordinate(arguments[0], coord1) && parseCoordinate(arguments[1], coord2)))
    return false;

  //calculate the radius by taking the average of the two distances (in km)
  radius = (dist(&_center, &coord1) + dist(&_center, &coord2)) / 2.0;

  //qDebug( "Radius=%fKm, Dist1=%f, Dist2=%f",
  //radius, dist(&_center, &coord1), dist(&_center, &coord2) );

  int lat = _center.x();
  int lon = _center.y();

  // grenzen 180 oder 90 beachten!
  double distLat = dist( lat, lon, lat + 10000, lon );
  double distLon = dist( lat, lon, lat, lon + 10000 );

  //qDebug( "distLat=%f, distLon=%f, radius=%fkm", distLat, distLon, radius );

  // get the angles by calculating the bearing from the centerpoint to the WGS84 coordinates
  angle1 = bearing(_center, coord1);
  angle2 = bearing(_center, coord2);

  // add the arc to the point array
  addArc( radius/(distLat/10000.), radius/(distLon/10000.), angle1, angle2 );
  return true;
}


void OpenAirParser::addCircle(const double& rLat, const double& rLon)
{
  double x, y, phi;
  int pai=asPA.count();
  asPA.resize(pai+360);
  // qDebug("rLat: %d, rLon:%d", rLat, rLon);

  for (int i=0; i<360; i+=1) {
    phi=double(i)/180.0*M_PI;
    x = cos(phi)*rLat;
    y = sin(phi)*rLon;
    x +=_center.x();
    y +=_center.y();
    asPA.setPoint(pai, QPoint(int(rint(x)), int(rint(y))));
    pai++;
  }
}


void OpenAirParser::addCircle(const double& radius)
{
  int lat = _center.x();
  int lon = _center.y();

  // Check limits 180 or 90 degrees?
  double distLat = dist( lat, lon, lat + 10000, lon );
  double distLon = dist( lat, lon, lat, lon + 10000 );

  double kmr = radius * MILE_kfl / 1000.;

  //qDebug( "distLat=%f, distLon=%f, radius=%fkm", distLat, distLon, kmr );

  addCircle( kmr/(distLat/10000.), kmr/(distLon/10000.) );  // kilometer/minute
}


#define STEP_WIDTH 1

void OpenAirParser::addArc(const double& rX, const double& rY,
                           double angle1, double angle2)
{
  //qDebug("addArc() dir=%d, a1=%f a2=%f",_direction, angle1*180/M_PI , angle2*180/M_PI );

  double x, y;
  int pai= asPA.count();

  if (_direction>0) {
    if (angle1>=angle2)
      angle2+=2.0*M_PI;
  } else {
    if (angle2>=angle1)
      angle1+=2.0*M_PI;
  }

  asPA.resize(pai+abs(int(((angle2-angle1)*180)/(STEP_WIDTH*M_PI)))+2);

  //qDebug("delta=%d pai=%d",int(((angle2-angle1)*180)/(STEP_WIDTH*M_PI)), pai );

  const double step=(STEP_WIDTH*M_PI)/180.0;

  if (_direction>0) { //clockwise
    for (double phi=angle1; phi<angle2; phi+=step) {
      x = ( cos(phi)*rX ) + _center.x();
      y = ( sin(phi)*rY ) + _center.y();

      /*
        QString xs = WGSPoint::printPos((int) rint(x), true );
        QString ys = WGSPoint::printPos((int) rint(y), false );

        qDebug("pai[%d], X=%f-%s, Y=%f-%s",
        pai,
        x, xs.toLatin1().data(),
        y, ys.toLatin1().data());
      */

      asPA.setPoint(pai, (int) rint(x), (int) rint(y));
      pai++;
    }
    x = ( cos(angle2)*rX ) + _center.x();
    y = ( sin(angle2)*rY ) + _center.y();
    asPA.setPoint(pai, (int) rint(x), (int) rint(y));
  } else {
    for (double phi=angle1; phi>angle2; phi-=step) {
      x = ( cos(phi)*rX ) + _center.x();
      y = ( sin(phi)*rY ) + _center.y();
      //qDebug("pai[%d], X=%f, Y=%f", pai, x, y);
      asPA.setPoint(pai, (int) rint(x), (int) rint(y));
      pai++;
    }
    x = ( cos(angle2)*rX ) + _center.x();
    y = ( sin(angle2)*rY ) + _center.y();
    asPA.setPoint(pai, (int) rint(x), (int) rint(y));
  }
}

/**
 * Read the content of a compiled file and put it into the passed
 * list.
 *
 * @param path Full name with path of OpenAir binary file
 * @param list All airspace objects have to be stored in this list
 * @returns true (success) or false (error occured)
 */
bool OpenAirParser::readCompiledFile( QString &path, Q3PtrList<Airspace>& list )
{
  QTime t;
  t.start();

  QFile inFile(path);

  if( !inFile.open(IO_ReadOnly) ) {
    qWarning("OpenAirParser: Cannot open airspace file %s!", path.toLatin1().data());
    return false;
  }

  QDataStream in(&inFile);

  // This was the order used by ealier cumulus
  // implementations. Because OpenAir does not support these all a
  // subset from the original implementation is only used to spare
  // memory and to get a better performance.

  quint32 magic;
  qint8 fileType;
  quint16 fileVersion;
  QDateTime creationDateTime;

#ifdef BOUNDING_BOX

  QRect boundingBox;
#endif

  ProjectionBase *projectionFromFile;
  qint32 buflen;

  in >> magic;

  if( magic != KFLOG_FILE_MAGIC ) {
    qWarning( "OpenAirParser: wrong magic key %x read! Aborting ...", h_magic );
    inFile.close();
    return false;
  }

  in >> fileType;

  if( fileType != FILE_TYPE_AIRSPACE_C ) {
    qWarning( "OpenAirParser: wrong file type %x read! Aborting ...", h_fileType );
    inFile.close();
    return false;
  }

  in >> fileVersion;

  if( fileVersion != FILE_VERSION_AIRSPACE_C ) {
    qWarning( "OpenAirParser: wrong file version %x read! Aborting ...", h_fileVersion );
    inFile.close();
    return false;
  }

  in >> creationDateTime;

  projectionFromFile = LoadProjection(in);

  // projectionFromFile is allocated dynamically, we don't need it
  // here. Therefore it is immediately deleted to avoid memory leaks.
  delete projectionFromFile;
  projectionFromFile = 0;

#ifdef BOUNDING_BOX

  in >> boundingBox;
#endif

  // because we're used a buffer during output, the buffer length will
  // be written too. Now we have to read it but don't need it because
  // we access directly to the opened file.
  in >> buflen;

  uint counter = 0;

  QString name;
  quint8 type;
  quint8 lowerType;
  qint16 lower;
  quint8 upperType;
  qint16 upper;
  QPolygon pa;

  while( ! in.atEnd() ) {
    counter++;
    pa.resize(0);

    ShortLoad(in, name);
    in >> type;
    in >> lowerType;
    in >> lower;
    in >> upperType;
    in >> upper;
    ShortLoad( in, pa );

    Airspace *a = new Airspace( name,
                                (BaseMapElement::objectType) type,
                                pa,
                                upper, (BaseMapElement::elevationType) upperType,
                                lower, (BaseMapElement::elevationType) lowerType );
    list.append(a);
  }

  inFile.close();

  //qDebug( "OpenAirParser: %d airspace objects read from file %s in %dms",
  //       counter, basename(path.toLatin1().data()), t.elapsed() );

  return true;
}


/**
 * Get the header data of a compiled file and put it in the class
 * variables.
 *
 * @param path Full name with path of OpenAir binary file
 * @returns true (success) or false (error occured)
 */
bool OpenAirParser::setHeaderData( QString &path )
{
  h_headerIsValid = false; // save read result here too

  h_magic = 0;
  h_fileType = 0;
  h_fileVersion = 0;

  if( h_projection ) {
    // delete an older projection object
    delete  h_projection;
    h_projection = 0;
  }

  QFile inFile(path);
  if( !inFile.open(IO_ReadOnly) ) {
    qWarning("OpenAirParser: Cannot open airspace file %s!", path.toLatin1().data());
    return false;
  }

  QDataStream in(&inFile);

  in >> h_magic;

  if( h_magic != KFLOG_FILE_MAGIC ) {
    qWarning( "OpenAirParser: wrong magic key %x read! Aborting ...", h_magic );
    inFile.close();
    return false;
  }

  in >> h_fileType;

  if( h_fileType != FILE_TYPE_AIRSPACE_C ) {
    qWarning( "OpenAirParser: wrong file type %x read! Aborting ...", h_fileType );
    inFile.close();
    return false;
  }

  in >> h_fileVersion;

  if( h_fileVersion != FILE_VERSION_AIRSPACE_C ) {
    qWarning( "OpenAirParser: wrong file version %x read! Aborting ...", h_fileVersion );
    inFile.close();
    return false;
  }

  in >> h_creationDateTime;

  h_projection = LoadProjection(in);

#ifdef BOUNDING_BOX
  in >> h_boundingBox;
#endif

  inFile.close();
  h_headerIsValid = true; // save read result here too
  return true;
}

