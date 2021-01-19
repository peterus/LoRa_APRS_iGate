#include <logger.h>
#include <TimeLib.h>
#include "project_configuration.h"
#include "TaskAprsIs.h"
#include "Task.h"

String create_lat_aprs(double lat);
String create_long_aprs(double lng);

AprsIsTask::AprsIsTask()
	: Task(TASK_APRS_IS, TaskAprsIs), _beacon_next_time(0)
{
}

AprsIsTask::~AprsIsTask()
{
}

bool AprsIsTask::setup(std::shared_ptr<Configuration> config, std::shared_ptr<BoardConfig> boardConfig)
{
	_aprs_is = std::shared_ptr<APRS_IS>(new APRS_IS(config->callsign, config->aprs_is.passcode , "ESP32-APRS-IS", "0.1"));

	_beaconMsg = std::shared_ptr<APRSMessage>(new APRSMessage());
	_beaconMsg->setSource(config->callsign);
	_beaconMsg->setDestination("APLG0");
	String lat = create_lat_aprs(config->beacon.positionLatitude);
	String lng = create_long_aprs(config->beacon.positionLongitude);
	_beaconMsg->getBody()->setData(String("=") + lat + "I" + lng + "&" + config->beacon.message);

	return true;
}

bool AprsIsTask::loop(std::shared_ptr<Configuration> config)
{
	if(!_aprs_is->connected())
	{
		return connect(config);
	}
	
	_aprs_is->getAPRSMessage();

	if(!inputQueue.empty())
	{
		std::shared_ptr<APRSMessage> msg = inputQueue.getElement();
		_aprs_is->sendMessage(msg);
	}

	if(_beacon_next_time < now())
	{
		logPrintD("[" + timeString() + "] ");
		logPrintlnD(_beaconMsg->encode());
		_aprs_is->sendMessage(_beaconMsg);
		_beacon_next_time = now() + config->beacon.timeout * 60UL;
	}
	return true;
}

bool AprsIsTask::connect(std::shared_ptr<Configuration> config)
{
	logPrintI("connecting to APRS-IS server: ");
	logPrintI(config->aprs_is.server);
	logPrintI(" on port: ");
	logPrintlnI(String(config->aprs_is.port));
	if(!_aprs_is->connect(config->aprs_is.server, config->aprs_is.port))
	{
		logPrintlnE("Connection failed.");
		return false;
	}
	logPrintlnI("Connected to APRS-IS server!");
	return true;
}