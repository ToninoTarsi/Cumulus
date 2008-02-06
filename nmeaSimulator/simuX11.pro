# NMEA Simulator for cumulus
# $Id$

TEMPLATE    = app
CONFIG      = qt warn_on release

HEADERS     = \
glider.h \
gpgga.h \
gprmc.h \
../cumulus/speed.h \
vector.h

SOURCES     = \
glider.cpp \
gpgga.cpp \
gprmc.cpp \
main.cpp \
../cumulus/speed.cpp \
vector.cpp \

TARGET = nmeaSimu
DESTDIR     = .
INCLUDEPATH += ../cumulus

LIBS += -lstdc++ -lm
