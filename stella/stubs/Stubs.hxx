#include <ctime>
#include "OSystem.hxx"

OSystem::OSystem()
{
    myNVRamDir     = ".";
    mySettings     = 0;
    mySound        = new Sound(this);
    mySerialPort   = new SerialPort();
    myEventHandler = new EventHandler(this);
    myPropSet      = new PropertiesSet(this);
    Paddles::setDigitalSensitivity(50);
    Paddles::setMouseSensitivity(5);
}

OSystem::~OSystem()
{
    delete mySound;
    delete mySerialPort;
    delete myEventHandler;
    delete myPropSet;
}

bool OSystem::create() { return 1; }
void OSystem::stateChanged(EventHandler::State state) { }

uInt64 OSystem::getTicks() const
{
    return myConsole->tia().getMilliSeconds();
}

EventHandler::EventHandler(OSystem*) { }
EventHandler::~EventHandler() { }
