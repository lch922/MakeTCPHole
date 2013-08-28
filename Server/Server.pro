QT += network sql

HEADERS += \
    serverform.h

SOURCES += \
    serverform.cpp \
    Server.cpp

FORMS += \
    serverform.ui

LIBS += -lWs2_32
