#pragma once

#if AP_XRCE_ENABLED

#include "uxr/client/client.h"
#include "ucdr/microcdr.h"
#include "AP_XRCE_ROS2_Builtin_Interfaces_Topics.h"

#include <AP_HAL/AP_HAL.h>
#include <AP_HAL/Scheduler.h>
#include <AP_HAL/Semaphores.h>
#include <AP_AHRS/AP_AHRS.h>

#include "fcntl.h"

#include <AP_Param/AP_Param.h>
#define STREAM_HISTORY 8
#define BUFFER_SIZE_SERIAL UXR_CONFIG_SERIAL_TRANSPORT_MTU * STREAM_HISTORY

extern const AP_HAL::HAL& hal;

class AP_XRCE_Client {

private:
    // Serial Device
    uint8_t fd;
    uint8_t relativeSerialAgentAddr;
    uint8_t relativeSerialClientAddr;

    uxrSerialTransport serial_transport; // client uxr serial transport
    uxrSession session; //Session

    // Input Stream
    uint8_t input_reliable_stream[BUFFER_SIZE_SERIAL];
    uxrStreamId reliable_in;

    // Output Stream
    uint8_t output_reliable_stream[BUFFER_SIZE_SERIAL];
    uxrStreamId reliable_out;

    // Create Participant
    uxrObjectId participant_id;
    uint16_t participant_req;

    // Topic
    uxrObjectId topic_id;
    uint16_t topic_req;

    ROS2_BuiltinInterfacesTimeTopic* xrce_topic;

    // Publisher
    uxrObjectId pub_id;
    uint16_t pub_req;

    // DataWriter
    uxrObjectId dwriter_id;
    uint16_t dwriter_req;

    //Status requests
    uint8_t status[4];

    HAL_Semaphore csem;

    // connection parametrics
    bool connected;

    AP_Int8 xrce_type;

public:
    // Constructor
    AP_XRCE_Client();

    bool init();
    bool create();
    void write();
    void update();
    static const struct AP_Param::GroupInfo var_info[];
};

#endif // AP_XRCE_ENABLED


