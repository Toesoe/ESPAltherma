#ifndef ESPALTHERMA_STAGE_H
#define ESPALTHERMA_STAGE_H

// Where the main loop, the heat pump task and the MQTT task are: when one of them gets stuck, the watchdog puts it
// in the restart cause (event history). Set before each step that can block.
volatile const char *loopStage = "starting";
volatile const char *hpStage = "starting";
volatile const char *mqttStage = "starting";

#define LOOP_STAGE(s) (loopStage = (s))
#define HP_STAGE(s) (hpStage = (s))
#define MQTT_STAGE(s) (mqttStage = (s))

// PubSubClient waits up to 10s on each write the broker does not take, and still reports "connected": the log
// alone is dozens of publishes a cycle, enough to freeze the main loop for minutes. After one failed write we
// stop publishing and the MQTT task drops the connection (it reconnects).
volatile bool mqttWriteFailed = false;

#endif
