#pragma once

#include <windows.h>
#include <wdf.h>
#include <SensorsCx.h>
#include <SensorsUtils.h>
#include <sensorsdef.h>

// Custom IOCTL codes for GPS coordinate updates
#define IOCTL_VIRTUAL_GPS_SET_COORDINATE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIRTUAL_GPS_GET_COORDINATE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

// GPS coordinate structure for IOCTL
typedef struct _GPS_COORDINATE_DATA {
    DOUBLE Latitude;
    DOUBLE Longitude;
    DOUBLE Altitude;
    DOUBLE ErrorRadius;
} GPS_COORDINATE_DATA, *PGPS_COORDINATE_DATA;

// Device context structure
typedef struct _DEVICE_CONTEXT {
    SENSOROBJECT SensorInstance;
    WDFWAITLOCK Lock;
    BOOLEAN Started;
    DOUBLE Latitude;
    DOUBLE Longitude;
    DOUBLE Altitude;
    DOUBLE ErrorRadius;
    WDFTIMER UpdateTimer;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext)

// Driver callbacks
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD VirtualGNSSEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP VirtualGNSSEvtDriverContextCleanup;

// Sensor callbacks
EVT_SENSOR_DRIVER_START_SENSOR VirtualGNSSEvtSensorStart;
EVT_SENSOR_DRIVER_STOP_SENSOR VirtualGNSSEvtSensorStop;
EVT_SENSOR_DRIVER_GET_SUPPORTED_DATA_FIELDS VirtualGNSSEvtGetSupportedDataFields;
EVT_SENSOR_DRIVER_GET_DATA_FIELD_PROPERTIES VirtualGNSSEvtGetDataFieldProperties;
EVT_SENSOR_DRIVER_GET_DATA_INTERVAL VirtualGNSSEvtGetDataInterval;
EVT_SENSOR_DRIVER_SET_DATA_INTERVAL VirtualGNSSEvtSetDataInterval;
EVT_SENSOR_DRIVER_GET_DATA_THRESHOLDS VirtualGNSSEvtGetDataThresholds;
EVT_SENSOR_DRIVER_SET_DATA_THRESHOLDS VirtualGNSSEvtSetDataThresholds;
EVT_SENSOR_DRIVER_DEVICE_IO_CONTROL VirtualGNSSEvtDeviceIoControl;

// Timer callback
EVT_WDF_TIMER VirtualGNSSEvtTimerFunc;

// Helper functions
NTSTATUS InitializeSensorInstance(WDFDEVICE Device);
NTSTATUS UpdateSensorData(PDEVICE_CONTEXT DeviceContext);
NTSTATUS CreateSensorDataCollection(
    PDEVICE_CONTEXT DeviceContext,
    PSENSOR_COLLECTION_LIST* ppData
);
VOID FreeSensorDataCollection(PSENSOR_COLLECTION_LIST pData);
