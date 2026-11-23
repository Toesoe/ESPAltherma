#ifndef mqttSerial_h
#define mqttSerial_h
#include "Stream.h"
#include <PubSubClient.h>
#if defined(ARDUINO_M5Stick_C_Plus2) || defined(ARDUINO_M5Stick_C_Plus) || defined(ARDUINO_M5Stick_C) || defined(ARDUINO_M5Stack_Tough)
#include <M5Unified.h>
#endif

#include "config.h"
#include "logbuf.h"
#include "stage.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// mqtt.h: the MQTT client runs in its own task, so the main loop only queues a message.
bool mqttPublish(const char *topic, const char *payload, bool retained);

// Log stream: written to Serial right away, and queued for MQTT (espaltherma/log) and the M5 screen.
// The queue is drained by the main loop, so logging is safe from any task.
class MQTTSerial: public Stream
{
private:
    /* data */
    PubSubClient* _client = nullptr;
    char _topic[64];
    SemaphoreHandle_t _topicMutex = xSemaphoreCreateMutex(); // setupMqttClient() may change it from the MQTT task
    LogRing _ring;
public:
    inline void begin(PubSubClient* client,const char* topic){
    _client=client;
    if (_topicMutex) xSemaphoreTake(_topicMutex, portMAX_DELAY);
    strlcpy(_topic,topic,sizeof(_topic));
    if (_topicMutex) xSemaphoreGive(_topicMutex);
    };

    inline size_t write(uint8_t){return 0;};
    size_t write(const uint8_t *buffer, size_t size);
    inline int available(void){return 0;};
    inline int availableForWrite(void){return 0;};
    inline int peek(void){return 0;};
    inline int read(void){return 0;};
    inline void flush(void){};
    inline size_t write(const char * s)
    {
        return write((uint8_t*) s, strlen(s));
    }
    inline size_t write(unsigned long n)
    {
        return write((uint8_t) n);
    }
    inline size_t write(long n)
    {
        return write((uint8_t) n);
    }
    inline size_t write(unsigned int n)
    {
        return write((uint8_t) n);
    }
    inline size_t write(int n)
    {
        return write((uint8_t) n);
    }


    // Forwards the queued log chunks to the screen and MQTT. Call from the main loop only.
    void drain();

    // Also receives every chunk drained (web interface log)
    void (*onChunk)(const char *chunk, size_t len) = nullptr;

    MQTTSerial();
    ~MQTTSerial();
};

MQTTSerial mqttSerial;

MQTTSerial::MQTTSerial()
{
}
size_t MQTTSerial::write(const uint8_t *buffer, size_t size)
{
    if (!config.disableLogMessages){
        Serial.write(buffer,size);
#ifdef SECOND_CONSOLE
        SECOND_CONSOLE.write(buffer,size);
#endif
    }
    _ring.push(buffer, size);
    return size;
}

void MQTTSerial::drain()
{
    char chunk[LogRing::MAX_CHUNK + 1];
    size_t len;
    while ((len = _ring.pop(chunk, sizeof(chunk))) > 0)
    {
#if defined(ARDUINO_M5Stick_C) || defined(ARDUINO_M5Stack_Tough)
        if (M5.Lcd.getCursorY()+13>M5.Lcd.height()){
            M5.Lcd.fillScreen(TFT_BLACK);
            M5.Lcd.setCursor(0,0);
        }
        M5.Lcd.print(chunk);
#endif
        if (!config.disableLogMessages && !mqttWriteFailed){ // connected over WiFi or Ethernet
            char topic[64];
            if (_topicMutex) xSemaphoreTake(_topicMutex, portMAX_DELAY);
            memcpy(topic, _topic, sizeof(topic));
            if (_topicMutex) xSemaphoreGive(_topicMutex);
            mqttPublish(topic, chunk, false); // the message is queued; only the MQTT task touches the client
        }
        if (onChunk != nullptr){
            onChunk(chunk, len);
        }
    }
}

MQTTSerial::~MQTTSerial()
{
}

#endif