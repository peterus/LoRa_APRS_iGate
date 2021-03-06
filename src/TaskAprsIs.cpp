#include <TimeLib.h>
#include <logger.h>

#include "Task.h"
#include "TaskAprsIs.h"
#include "project_configuration.h"

String create_lat_aprs(double lat);
String create_long_aprs(double lng);

AprsIsTask::AprsIsTask() : Task(TASK_APRS_IS, TaskAprsIs) {
}

AprsIsTask::~AprsIsTask() {
}

bool AprsIsTask::setup(std::shared_ptr<System> system) {
  _beacon_timer.setTimeout(system->getUserConfig()->beacon.timeout * 60 * 1000);
  _aprs_is = std::shared_ptr<APRS_IS>(new APRS_IS(system->getUserConfig()->callsign, system->getUserConfig()->aprs_is.passcode, "ESP32-APRS-IS", "0.2"));

  _beaconMsg = std::shared_ptr<APRSMessage>(new APRSMessage());
  _beaconMsg->setSource(system->getUserConfig()->callsign);
  _beaconMsg->setDestination("APLG01");
  String lat = create_lat_aprs(system->getUserConfig()->beacon.positionLatitude);
  String lng = create_long_aprs(system->getUserConfig()->beacon.positionLongitude);
  _beaconMsg->getBody()->setData(String("=") + lat + "L" + lng + "&" + system->getUserConfig()->beacon.message);

  return true;
}

bool AprsIsTask::loop(std::shared_ptr<System> system) {
  if (!system->isWifiEthConnected()) {
    return false;
  }
  if (!_aprs_is->connected()) {
    if (!connect(system)) {
      _stateInfo = "not connected";
      _state     = Error;
      return false;
    }
    _stateInfo = "connected";
    _state     = Okay;
    return false;
  }

  _aprs_is->getAPRSMessage();

  if (!inputQueue.empty()) {
    std::shared_ptr<APRSMessage> msg = inputQueue.getElement();
    _aprs_is->sendMessage(msg);
  }

  if (_beacon_timer.check()) {
    logPrintD("[" + timeString() + "] ");
    logPrintlnD(_beaconMsg->encode());
    _aprs_is->sendMessage(_beaconMsg);
    system->getDisplay().addFrame(std::shared_ptr<DisplayFrame>(new TextFrame("BEACON", _beaconMsg->toString())));
    _beacon_timer.start();
  }
  time_t diff = _beacon_timer.getTriggerTimeInSec();
  _stateInfo  = "beacon " + String(diff / 60) + ":" + String(diff % 60);
  _state      = Okay;
  return true;
}

bool AprsIsTask::connect(std::shared_ptr<System> system) {
  logPrintI("connecting to APRS-IS server: ");
  logPrintI(system->getUserConfig()->aprs_is.server);
  logPrintI(" on port: ");
  logPrintlnI(String(system->getUserConfig()->aprs_is.port));
  if (!_aprs_is->connect(system->getUserConfig()->aprs_is.server, system->getUserConfig()->aprs_is.port)) {
    logPrintlnE("Connection failed.");
    return false;
  }
  logPrintlnI("Connected to APRS-IS server!");
  return true;
}
