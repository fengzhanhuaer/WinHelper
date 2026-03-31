#include "virtual_gnss_driver.h"
#include <initguid.h>
#include <devpkey.h>

// GNSS sensor type GUID
DEFINE_GUID(GUID_SensorType_GeomagneticOrientation,
    0x9494A3B0, 0x362D, 0x4B26, 0xB2, 0xE0, 0xF1, 0xB3, 0xA5, 0xC8, 0x6B, 0x8D);

// Default coordinate (1 S Market St, San Jose, CA)
#define DEFAULT_LATITUDE  37.3337
#define DEFAULT_LONGITUDE -121.8907
#define DEFAULT_ALTITUDE  25.0
#define DEFAULT_ERROR_RADIUS 10.0

// Update interval in milliseconds
#define DEFAULT_UPDATE_INTERVAL_MS 1000

// ---------------------------------------------------------------------------
// Driver Entry
// ---------------------------------------------------------------------------
NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
) {
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    WDF_DRIVER_CONFIG_INIT(&config, VirtualGNSSEvtDeviceAdd);
    config.EvtDriverUnload = VirtualGNSSEvtDriverContextCleanup;

    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES,
                            &config, WDF_NO_HANDLE);

    return status;
}

// ---------------------------------------------------------------------------
// Device Add
// ---------------------------------------------------------------------------
NTSTATUS VirtualGNSSEvtDeviceAdd(
    _In_ WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
) {
    NTSTATUS status;
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    PDEVICE_CONTEXT deviceContext;
    SENSOR_CONTROLLER_CONFIG sensorConfig;
    WDF_OBJECT_ATTRIBUTES timerAttributes;
    WDF_TIMER_CONFIG timerConfig;

    UNREFERENCED_PARAMETER(Driver);

    // Configure sensor
    SENSOR_CONTROLLER_CONFIG_INIT(&sensorConfig);
    sensorConfig.DriverIsPowerPolicyOwner = WdfUseDefault;

    status = SensorsCxDeviceInitConfig(DeviceInit, &sensorConfig, 0);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Create device
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);
    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Initialize device context
    deviceContext = GetDeviceContext(device);
    deviceContext->Started = FALSE;
    deviceContext->Latitude = DEFAULT_LATITUDE;
    deviceContext->Longitude = DEFAULT_LONGITUDE;
    deviceContext->Altitude = DEFAULT_ALTITUDE;
    deviceContext->ErrorRadius = DEFAULT_ERROR_RADIUS;

    status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &deviceContext->Lock);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Create timer for periodic updates
    WDF_TIMER_CONFIG_INIT(&timerConfig, VirtualGNSSEvtTimerFunc);
    timerConfig.Period = DEFAULT_UPDATE_INTERVAL_MS;
    timerConfig.AutomaticSerialization = TRUE;

    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
    timerAttributes.ParentObject = device;

    status = WdfTimerCreate(&timerConfig, &timerAttributes, &deviceContext->UpdateTimer);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Initialize sensor instance
    status = InitializeSensorInstance(device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return STATUS_SUCCESS;
}

// ---------------------------------------------------------------------------
// Initialize Sensor Instance
// ---------------------------------------------------------------------------
NTSTATUS InitializeSensorInstance(WDFDEVICE Device) {
    PDEVICE_CONTEXT deviceContext = GetDeviceContext(Device);
    SENSOR_CONFIG sensorConfig;
    NTSTATUS status;

    SENSOR_CONFIG_INIT(&sensorConfig);
    sensorConfig.pEnumerationList = nullptr;

    // Set sensor callbacks
    sensorConfig.pEvtSensorStart = VirtualGNSSEvtSensorStart;
    sensorConfig.pEvtSensorStop = VirtualGNSSEvtSensorStop;
    sensorConfig.pEvtSensorGetSupportedDataFields = VirtualGNSSEvtGetSupportedDataFields;
    sensorConfig.pEvtSensorGetDataFieldProperties = VirtualGNSSEvtGetDataFieldProperties;
    sensorConfig.pEvtSensorGetDataInterval = VirtualGNSSEvtGetDataInterval;
    sensorConfig.pEvtSensorSetDataInterval = VirtualGNSSEvtSetDataInterval;
    sensorConfig.pEvtSensorGetDataThresholds = VirtualGNSSEvtGetDataThresholds;
    sensorConfig.pEvtSensorSetDataThresholds = VirtualGNSSEvtSetDataThresholds;
    sensorConfig.pEvtSensorDeviceIoControl = VirtualGNSSEvtDeviceIoControl;

    status = SensorsCxSensorCreate(Device, WDF_NO_OBJECT_ATTRIBUTES,
                                   &deviceContext->SensorInstance);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = SensorsCxSensorInitialize(deviceContext->SensorInstance, &sensorConfig);

    return status;
}

// ---------------------------------------------------------------------------
// Timer Callback - Periodic Data Updates
// ---------------------------------------------------------------------------
VOID VirtualGNSSEvtTimerFunc(_In_ WDFTIMER Timer) {
    WDFDEVICE device = (WDFDEVICE)WdfTimerGetParentObject(Timer);
    PDEVICE_CONTEXT deviceContext = GetDeviceContext(device);

    if (deviceContext->Started) {
        UpdateSensorData(deviceContext);
    }
}

// ---------------------------------------------------------------------------
// Create Sensor Data Collection
// ---------------------------------------------------------------------------
NTSTATUS CreateSensorDataCollection(
    PDEVICE_CONTEXT DeviceContext,
    PSENSOR_COLLECTION_LIST* ppData
) {
    NTSTATUS status = STATUS_SUCCESS;
    PSENSOR_COLLECTION_LIST pData = nullptr;
    ULONG dataSize;

    // Calculate size needed for collection (only position data, no timestamp)
    dataSize = SENSOR_COLLECTION_LIST_SIZE(4); // Latitude, Longitude, Altitude, ErrorRadius

    pData = (PSENSOR_COLLECTION_LIST)ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        dataSize,
        'SPGV'
    );

    if (pData == nullptr) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pData, dataSize);
    pData->Count = 0;
    pData->AllocatedSizeInBytes = dataSize;

    // Add Latitude
    pData->List[pData->Count].Key = SENSOR_DATA_TYPE_LATITUDE_DEGREES;
    InitPropVariantFromDouble(DeviceContext->Latitude, &pData->List[pData->Count].Value);
    pData->Count++;

    // Add Longitude
    pData->List[pData->Count].Key = SENSOR_DATA_TYPE_LONGITUDE_DEGREES;
    InitPropVariantFromDouble(DeviceContext->Longitude, &pData->List[pData->Count].Value);
    pData->Count++;

    // Add Altitude
    pData->List[pData->Count].Key = SENSOR_DATA_TYPE_ALTITUDE_ELLIPSOID_METERS;
    InitPropVariantFromDouble(DeviceContext->Altitude, &pData->List[pData->Count].Value);
    pData->Count++;

    // Add Error Radius
    pData->List[pData->Count].Key = SENSOR_DATA_TYPE_ERROR_RADIUS_METERS;
    InitPropVariantFromDouble(DeviceContext->ErrorRadius, &pData->List[pData->Count].Value);
    pData->Count++;

    *ppData = pData;
    return status;
}

// ---------------------------------------------------------------------------
// Free Sensor Data Collection
// ---------------------------------------------------------------------------
VOID FreeSensorDataCollection(PSENSOR_COLLECTION_LIST pData) {
    if (pData != nullptr) {
        for (ULONG i = 0; i < pData->Count; i++) {
            PropVariantClear(&pData->List[i].Value);
        }
        ExFreePoolWithTag(pData, 'SPGV');
    }
}

// ---------------------------------------------------------------------------
// Update Sensor Data
// ---------------------------------------------------------------------------
NTSTATUS UpdateSensorData(PDEVICE_CONTEXT DeviceContext) {
    NTSTATUS status = STATUS_SUCCESS;
    PSENSOR_COLLECTION_LIST pData = nullptr;

    WdfWaitLockAcquire(DeviceContext->Lock, nullptr);

    if (DeviceContext->Started) {
        status = CreateSensorDataCollection(DeviceContext, &pData);
        
        if (NT_SUCCESS(status) && pData != nullptr) {
            // Report data to sensor class extension
            status = SensorsCxSensorDataReady(DeviceContext->SensorInstance, pData);
            FreeSensorDataCollection(pData);
        }
    }

    WdfWaitLockRelease(DeviceContext->Lock);

    return status;
}

// ---------------------------------------------------------------------------
// Sensor Callbacks
// ---------------------------------------------------------------------------
NTSTATUS VirtualGNSSEvtSensorStart(_In_ SENSOROBJECT SensorInstance) {
    WDFDEVICE device = WdfObjectContextGetObject(SensorInstance);
    PDEVICE_CONTEXT deviceContext = GetDeviceContext(device);

    WdfWaitLockAcquire(deviceContext->Lock, nullptr);
    deviceContext->Started = TRUE;
    WdfWaitLockRelease(deviceContext->Lock);

    // Start timer for periodic updates
    WdfTimerStart(deviceContext->UpdateTimer, WDF_REL_TIMEOUT_IN_MS(DEFAULT_UPDATE_INTERVAL_MS));

    // Send initial data
    UpdateSensorData(deviceContext);

    return STATUS_SUCCESS;
}

NTSTATUS VirtualGNSSEvtSensorStop(_In_ SENSOROBJECT SensorInstance) {
    WDFDEVICE device = WdfObjectContextGetObject(SensorInstance);
    PDEVICE_CONTEXT deviceContext = GetDeviceContext(device);

    // Stop timer
    WdfTimerStop(deviceContext->UpdateTimer, TRUE);

    WdfWaitLockAcquire(deviceContext->Lock, nullptr);
    deviceContext->Started = FALSE;
    WdfWaitLockRelease(deviceContext->Lock);

    return STATUS_SUCCESS;
}

NTSTATUS VirtualGNSSEvtGetSupportedDataFields(
    _In_ SENSOROBJECT SensorInstance,
    _Out_ PSENSOR_PROPERTY_LIST* pFields
) {
    PSENSOR_PROPERTY_LIST pList;
    ULONG size;

    UNREFERENCED_PARAMETER(SensorInstance);

    size = SENSOR_PROPERTY_LIST_SIZE(4);
    pList = (PSENSOR_PROPERTY_LIST)ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        size,
        'SPGV'
    );

    if (pList == nullptr) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pList, size);
    pList->Count = 4;
    pList->AllocatedSizeInBytes = size;

    pList->List[0] = SENSOR_DATA_TYPE_LATITUDE_DEGREES;
    pList->List[1] = SENSOR_DATA_TYPE_LONGITUDE_DEGREES;
    pList->List[2] = SENSOR_DATA_TYPE_ALTITUDE_ELLIPSOID_METERS;
    pList->List[3] = SENSOR_DATA_TYPE_ERROR_RADIUS_METERS;

    *pFields = pList;
    return STATUS_SUCCESS;
}

NTSTATUS VirtualGNSSEvtGetDataFieldProperties(
    _In_ SENSOROBJECT SensorInstance,
    _In_ const PROPERTYKEY* DataField,
    _Out_ PSENSOR_COLLECTION_LIST* pProperties
) {
    UNREFERENCED_PARAMETER(SensorInstance);
    UNREFERENCED_PARAMETER(DataField);
    UNREFERENCED_PARAMETER(pProperties);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS VirtualGNSSEvtGetDataInterval(
    _In_ SENSOROBJECT SensorInstance,
    _Out_ PULONG pDataRateMs
) {
    UNREFERENCED_PARAMETER(SensorInstance);
    *pDataRateMs = DEFAULT_UPDATE_INTERVAL_MS;
    return STATUS_SUCCESS;
}

NTSTATUS VirtualGNSSEvtSetDataInterval(
    _In_ SENSOROBJECT SensorInstance,
    _In_ ULONG DataRateMs
) {
    WDFDEVICE device = WdfObjectContextGetObject(SensorInstance);
    PDEVICE_CONTEXT deviceContext = GetDeviceContext(device);

    // Update timer period if sensor is running
    if (deviceContext->Started && DataRateMs > 0) {
        WdfTimerStop(deviceContext->UpdateTimer, TRUE);
        WdfTimerStart(deviceContext->UpdateTimer, WDF_REL_TIMEOUT_IN_MS(DataRateMs));
    }

    return STATUS_SUCCESS;
}

NTSTATUS VirtualGNSSEvtGetDataThresholds(
    _In_ SENSOROBJECT SensorInstance,
    _Out_ PSENSOR_COLLECTION_LIST* pThresholds
) {
    UNREFERENCED_PARAMETER(SensorInstance);
    UNREFERENCED_PARAMETER(pThresholds);
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS VirtualGNSSEvtSetDataThresholds(
    _In_ SENSOROBJECT SensorInstance,
    _In_ PSENSOR_COLLECTION_LIST pThresholds
) {
    UNREFERENCED_PARAMETER(SensorInstance);
    UNREFERENCED_PARAMETER(pThresholds);
    return STATUS_SUCCESS;
}

// ---------------------------------------------------------------------------
// IOCTL Handler - Service Communication
// ---------------------------------------------------------------------------
NTSTATUS VirtualGNSSEvtDeviceIoControl(
    _In_ SENSOROBJECT SensorInstance,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
) {
    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE device = WdfObjectContextGetObject(SensorInstance);
    PDEVICE_CONTEXT deviceContext = GetDeviceContext(device);
    PGPS_COORDINATE_DATA pInputData = nullptr;
    PGPS_COORDINATE_DATA pOutputData = nullptr;
    size_t bytesReturned = 0;

    switch (IoControlCode) {
    case IOCTL_VIRTUAL_GPS_SET_COORDINATE:
        // Set new GPS coordinates from service
        if (InputBufferLength < sizeof(GPS_COORDINATE_DATA)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveInputBuffer(
            Request,
            sizeof(GPS_COORDINATE_DATA),
            (PVOID*)&pInputData,
            nullptr
        );

        if (NT_SUCCESS(status)) {
            WdfWaitLockAcquire(deviceContext->Lock, nullptr);
            
            deviceContext->Latitude = pInputData->Latitude;
            deviceContext->Longitude = pInputData->Longitude;
            deviceContext->Altitude = pInputData->Altitude;
            deviceContext->ErrorRadius = pInputData->ErrorRadius;
            
            WdfWaitLockRelease(deviceContext->Lock);

            // Immediately update sensor data
            if (deviceContext->Started) {
                UpdateSensorData(deviceContext);
            }
        }
        break;

    case IOCTL_VIRTUAL_GPS_GET_COORDINATE:
        // Get current GPS coordinates
        if (OutputBufferLength < sizeof(GPS_COORDINATE_DATA)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveOutputBuffer(
            Request,
            sizeof(GPS_COORDINATE_DATA),
            (PVOID*)&pOutputData,
            nullptr
        );

        if (NT_SUCCESS(status)) {
            WdfWaitLockAcquire(deviceContext->Lock, nullptr);
            
            pOutputData->Latitude = deviceContext->Latitude;
            pOutputData->Longitude = deviceContext->Longitude;
            pOutputData->Altitude = deviceContext->Altitude;
            pOutputData->ErrorRadius = deviceContext->ErrorRadius;
            
            WdfWaitLockRelease(deviceContext->Lock);

            bytesReturned = sizeof(GPS_COORDINATE_DATA);
        }
        break;

    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, bytesReturned);
    return status;
}

VOID VirtualGNSSEvtDriverContextCleanup(_In_ WDFOBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);
}
