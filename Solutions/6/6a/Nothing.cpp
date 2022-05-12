//
// Copyright 2007-2022 OSR Open Systems Resources, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from this
//    software without specific prior written permission.
// 
//    This software is supplied for instructional purposes only.  It is not
//    complete, and it is not suitable for use in any production environment.
//
//    OSR Open Systems Resources, Inc. (OSR) expressly disclaims any warranty
//    for this software.  THIS SOFTWARE IS PROVIDED  "AS IS" WITHOUT WARRANTY
//    OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION,
//    THE IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR
//    PURPOSE.  THE ENTIRE RISK ARISING FROM THE USE OF THIS SOFTWARE REMAINS
//    WITH YOU.  OSR's entire liability and your exclusive remedy shall not
//    exceed the price paid for this material.  In no event shall OSR or its
//    suppliers be liable for any damages whatsoever (including, without
//    limitation, damages for loss of business profit, business interruption,
//    loss of business information, or any other pecuniary loss) arising out
//    of the use or inability to use this software, even if OSR has been
//    advised of the possibility of such damages.  Because some states/
//    jurisdictions do not allow the exclusion or limitation of liability for
//    consequential or incidental damages, the above limitation may not apply
//    to you.
// 

#include "nothing.h"

///////////////////////////////////////////////////////////////////////////////
//
//  DriverEntry
//
//    This routine is called by Windows when the driver is first loaded.  It
//    is the responsibility of this routine to create the WDFDRIVER
//
//  INPUTS:
//
//      DriverObject - Address of the DRIVER_OBJECT created by Windows for this
//                     driver.
//
//      RegistryPath - UNICODE_STRING which represents this driver's key in the
//                     Registry.  
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      STATUS_SUCCESS, otherwise an error indicating why the driver could not
//                      load.
//
//  IRQL:
//
//      This routine is called at IRQL == PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
NTSTATUS
DriverEntry(PDRIVER_OBJECT  DriverObject,
            PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG driverConfig;
    NTSTATUS          status;

#if DBG
    DbgPrint("\nOSR Nothing Driver -- Compiled %s %s\n",
             __DATE__,
             __TIME__);
#endif

    //
    // Provide pointer to our EvtDeviceAdd event processing callback
    // function
    //
    WDF_DRIVER_CONFIG_INIT(&driverConfig,
                           NothingEvtDeviceAdd);

    //
    // Create our WDFDriver instance
    //
    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &driverConfig,
                             WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("WdfDriverCreate failed 0x%0x\n",
                 status);
#endif
    }

    return (status);
}

///////////////////////////////////////////////////////////////////////////////
//
//  NothingEvtDeviceAdd
//
//    This routine is called by the framework when a device of
//    the type we support is found in the system.
//
//  INPUTS:
//
//      DriverObject - Our WDFDRIVER object
//
//      DeviceInit   - The device initialization structure we'll
//                     be using to create our WDFDEVICE
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      STATUS_SUCCESS, otherwise an error indicating why the driver could not
//                      load.
//
//  IRQL:
//
//      This routine is called at IRQL == PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
NTSTATUS
NothingEvtDeviceAdd(WDFDRIVER       Driver,
                    PWDFDEVICE_INIT DeviceInit)
{
    NTSTATUS                     status;
    WDF_OBJECT_ATTRIBUTES        objAttributes;
    WDFDEVICE                    device;
    WDF_IO_QUEUE_CONFIG          queueConfig;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    PNOTHING_DEVICE_CONTEXT      devContext;
    WDF_FILEOBJECT_CONFIG        fileObjectConfig;


    DECLARE_CONST_UNICODE_STRING(userDeviceName,
                                 L"\\DosDevices\\Nothing");

    UNREFERENCED_PARAMETER(Driver);

    //
    // Life is nice and simple in this driver... 
    //
    // We don't need ANY PnP/Power Event Processing callbacks. There's no
    // hardware, thus we don't need EvtPrepareHardware or EvtReleaseHardware 
    // There's no power state to handle so we don't need EvtD0Entry or EvtD0Exit.
    //
    // However, for this exercise we'll be adding D0Entry and D0Exit callbacks
    // for fun.
    //


    //
    // Prepare for WDFDEVICE creation
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&objAttributes);

    //
    // Specify our device context
    //
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&objAttributes,
                                           NOTHING_DEVICE_CONTEXT);

    //
    // We're adding D0Entry/D0Exit callbacks, so we need to
    // initialize out PnP power event callbacks structure.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    //
    // Fill in the D0Entry callback.
    //
    pnpPowerCallbacks.EvtDeviceD0Entry = NothingEvtDeviceD0Entry;

    //
    // Fill in the D0Exit callback
    //
    pnpPowerCallbacks.EvtDeviceD0Exit = NothingEvtDeviceD0Exit;

    //
    // Update the DeviceInit structure to contain the new callbacks.
    //
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit,
                                           &pnpPowerCallbacks);

    //
    // For Lab 6: We want to keep track of the first person to open us, so we
    // need EvtDeviceFileCreate and EvtFileClose callbacks.
    //
    WDF_FILEOBJECT_CONFIG_INIT(&fileObjectConfig,
                               NothingEvtFdoCreate,
                               NothingEvtFdoClose,
                               WDF_NO_EVENT_CALLBACK);

    //
    // Update the device init structure to specify our new
    // callbacks.
    //
    WdfDeviceInitSetFileObjectConfig(DeviceInit,
                                     &fileObjectConfig,
                                     WDF_NO_OBJECT_ATTRIBUTES);


    //
    // Create our device object
    //
    status = WdfDeviceCreate(&DeviceInit,
                             &objAttributes,
                             &device);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("WdfDeviceCreate failed 0x%0x\n",
                 status);
#endif
        goto Done;
    }

    //
    // Create a symbolic link to our Device Object.  This allows apps
    // to open our device by name.  Note that we use a constant name,
    // so this driver will support only a single instance of our device.
    //
    status = WdfDeviceCreateSymbolicLink(device,
                                         &userDeviceName);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("WdfDeviceCreateSymbolicLink failed 0x%0x\n",
                 status);
#endif
        goto Done;
    }

    //
    // ALSO create a device interface for the device
    // This allows usage of the SetupApiXxxx functions to locate
    // the device
    //
    status = WdfDeviceCreateDeviceInterface(device,
                                            &GUID_DEVINTERFACE_NOTHING,
                                            nullptr);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("WdfDeviceCreateDeviceInterface failed 0x%0x\n",
                 status);
#endif
        return (status);
    }

    //
    // Configure our Queue of incoming requests
    //
    // We use only the default Queue, and we set it for sequential processing.
    // This means that the driver will only receive one request at a time
    // from the Queue, and will not get another request until it completes
    // the previous one.
    //
    // Note for Lab 3B: A Queue with Sequential Dispatching ALSO releases a new
    // Request when an existing Request is forwarded to another Queue.  So, we
    // don't have to change the Dispatch Type to Parallel.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig,
                                           WdfIoQueueDispatchSequential);

    //
    // Declare our I/O Event Processing callbacks
    //
    // We handle, read, write, and device control requests.
    //
    // WDF will automagically handle Create and Close requests for us and will
    // will complete any other request types with STATUS_INVALID_DEVICE_REQUEST.    
    //
    queueConfig.EvtIoRead          = NothingEvtRead;
    queueConfig.EvtIoWrite         = NothingEvtWrite;
    queueConfig.EvtIoDeviceControl = NothingEvtDeviceControl;

    //
    // Because this is a queue for a software-only device, indicate
    // that the queue doesn't need to be power managed.
    //
    queueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(device,
                              &queueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              WDF_NO_HANDLE);

    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("WdfIoQueueCreate for default queue failed 0x%0x\n",
                 status);
#endif
        goto Done;
    }

    //
    // For Lab 3B:  We're now going to create our two manual Queues.
    // We're going to use one Queue to hold read Requests that are
    // waiting for a write Request to arrive;  We're going to use the
    // other Queue to hold read Requests that are waiting for a Write
    // to arrive. Because we only will be holding Requests of one type
    // or the other, we COULD have an optimization that uses only one
    // manual Queue (to hold either the waiting read or write Request)
    // but to keep things easy and clear in this example we'll use two
    // Queues.
    //
    devContext = NothingGetContextFromDevice(device);

    //
    // First the read...
    //
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
                             WdfIoQueueDispatchManual);

    status = WdfIoQueueCreate(device,
                              &queueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &devContext->ReadQueue);

    if (!NT_SUCCESS(status)) {

#if DBG
        DbgPrint("WdfIoQueueCreate for manual read queue failed 0x%0x\n",
                 status);
#endif
        goto Done;
    }


    //
    // Now the write...
    //
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
                             WdfIoQueueDispatchManual);

    status = WdfIoQueueCreate(device,
                              &queueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &devContext->WriteQueue);

    if (!NT_SUCCESS(status)) {

#if DBG
        DbgPrint("WdfIoQueueCreate for manual write queue failed 0x%0x\n",
                 status);
#endif

        goto Done;
    }

    //
    // The Collection we're about to create holds "stuff" that's specific
    // to a device instance.  So, we want to parent the Collection on the
    // WDFDEVICE Object. If we don't specify this, the Collection will be
    // parented on the WDFDRIVER.  It will *eventually* go away, but we
    // shouldn't have to wait for the driver to unload for device-specific
    // stuff to be destructed.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&objAttributes);

    objAttributes.ParentObject = device;

    //
    // Create the collection we use for keeping track of
    // WDFFILEOBJECTs.
    //
    status = WdfCollectionCreate(&objAttributes,
                                 &devContext->FileObjectsCollection);

    if (!NT_SUCCESS(status)) {

#if DBG
        DbgPrint("WdfCollectionCreate failed 0x%0x\n",
                 status);
#endif
        goto Done;
    }


    //
    // Like above, this Object is device specific, so we want
    // it to be parented on the Device.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&objAttributes);

    objAttributes.ParentObject = device;

    status = WdfSpinLockCreate(&objAttributes,
                               &devContext->FileObjectsCollectionLock);


    if (!NT_SUCCESS(status)) {

#if DBG
        DbgPrint("WdfSpinLockCreate failed 0x%0x\n",
                 status);
#endif
        goto Done;
    }

    status = STATUS_SUCCESS;

Done:

    return (status);
}

///////////////////////////////////////////////////////////////////////////////
//
//  NothingEvtDeviceD0Entry
//
//    This routine is called by the framework when a device of
//    the type we support is entering the D0 state
//
//  INPUTS:
//
//      Device       - One of our WDFDEVICE objects
//
//      PreviousState - The D-State we're entering D0 from
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      STATUS_SUCCESS, otherwise an error indicating why the driver could not
//                      load.
//
//  IRQL:
//
//      This routine is called at IRQL == PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
NTSTATUS
NothingEvtDeviceD0Entry(WDFDEVICE              Device,
                        WDF_POWER_DEVICE_STATE PreviousState)
{
    UNREFERENCED_PARAMETER(Device);

#if DBG
    DbgPrint("NothingEvtDeviceD0Entry - Entering D0 from state %s (%d)\n",
             NothingPowerDeviceStateToString(PreviousState),
             PreviousState);
#endif

    return STATUS_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
//
//  NothingEvtDeviceD0Exit
//
//    This routine is called by the framework when a device of
//    the type we support is leaving the D0 state
//
//  INPUTS:
//
//      Device       - One of our WDFDEVICE objects
//
//      TargetState  - The D-State we're entering
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      STATUS_SUCCESS, otherwise an error indicating why the driver could not
//                      load.
//
//  IRQL:
//
//      This routine is called at IRQL == PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
NTSTATUS
NothingEvtDeviceD0Exit(WDFDEVICE              Device,
                       WDF_POWER_DEVICE_STATE TargetState)
{
    UNREFERENCED_PARAMETER(Device);

#if DBG
    DbgPrint("NothingEvtDeviceD0Exit - Entering state %s (%d)\n",
             NothingPowerDeviceStateToString(TargetState),
             TargetState);
#endif

    return STATUS_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
//
//  NothingEvtRead
//
//    This routine is called by the framework when there is a
//    read request for us to process
//
//  INPUTS:
//
//      Queue    - Our default queue
//
//      Request  - A read request
//
//      Length   - The length of the read operation
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//  IRQL:
//
//      This routine is called at IRQL <= DISPATCH_LEVEL
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
VOID
NothingEvtRead(WDFQUEUE   Queue,
               WDFREQUEST Request,
               size_t     Length)
{
    PNOTHING_DEVICE_CONTEXT devContext;
    NTSTATUS                status;
    WDFREQUEST              writeRequest;
    PVOID                   writeBuffer;
    size_t                  writeBufferLen;
    PVOID                   readBuffer;
    size_t                  readBufferLen;
    size_t                  copyLen;

    UNREFERENCED_PARAMETER(Length);

#if DBG
    DbgPrint("NothingEvtRead\n");
#endif

    //
    // Get a pointer to our device context.
    // (get the WDFDEVICE from the WDFQUEUE, and the extension from the device)
    //
    devContext = NothingGetContextFromDevice(
                                             WdfIoQueueGetDevice(Queue));

    //
    // We have received a READ.  Are there any writes waiting?
    //
    status = WdfIoQueueRetrieveNextRequest(devContext->WriteQueue,
                                           &writeRequest);

    if (!NT_SUCCESS(status)) {

#if DBG
        DbgPrint("Failed to retrieve from the write queue (0x%x). "
                 "Queuing read\n",
                 status);
#endif

        //
        // There are no writes waiting.
        //
        // Put the read Request we just received onto the read Queue and return
        // with that read Request pending.
        //
        status = WdfRequestForwardToIoQueue(Request,
                                            devContext->ReadQueue);


        if (!NT_SUCCESS(status)) {

            //
            // Wow... we were unable to put the read Request onto the manual read Queue.
            // Not sure how we could get an error here, but... 
            //
#if DBG
            DbgPrint("WdfRequestForwardToIoQueue failed with Status code 0x%x",
                     status);
#endif
            copyLen = 0;

            goto DoneCompleteRead;
        }

        goto DoneJustReturn;
    }

    //
    // We DO have a write! Satisfy the read we just received using the
    // data from the write that we just removed from the Queue.
    //
#if DBG
    DbgPrint("Write request was pending!\n");
#endif

    //
    // Get the write buffer...
    //
    status = WdfRequestRetrieveInputBuffer(writeRequest,
                                           1,
                                           &writeBuffer,
                                           &writeBufferLen);
    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("WdfRequestRetrieveInputBuffer failed with Status code 0x%x",
                 status);
#endif

        //
        // Bad buffer... Fail both requests.
        //

        copyLen = 0;

        goto DoneCompleteBoth;
    }

    //
    // Get the read buffer...
    //
    status = WdfRequestRetrieveOutputBuffer(Request,
                                            1,
                                            &readBuffer,
                                            &readBufferLen);
    if (!NT_SUCCESS(status)) {

#if DBG
        DbgPrint("WdfRequestRetrieveOutputBuffer failed with Status code 0x%x",
                 status);
#endif
        //
        // Sigh... Fail both requests.
        //

        copyLen = 0;

        goto DoneCompleteBoth;
    }

    //
    // Only copy as much as the smaller of the two buffers. Sure, we
    // "lose" some data, but good enough for this exercise.
    //
    copyLen = min(writeBufferLen,
                  readBufferLen);

    //
    // Store the data from the write into the user's read buffer
    //
    RtlCopyMemory(readBuffer,
                  writeBuffer,
                  copyLen);

DoneCompleteBoth:

#if DBG
    DbgPrint("Completing requests with 0x%Ix bytes transferred\n",
             copyLen);
#endif

    //
    // Done with the write
    //
    WdfRequestCompleteWithInformation(writeRequest,
                                      status,
                                      copyLen);
DoneCompleteRead:

    //
    // Done with the read
    //
    WdfRequestCompleteWithInformation(Request,
                                      status,
                                      copyLen);
DoneJustReturn:

    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  NothingEvtWrite
//
//    This routine is called by the framework when there is a
//    write request for us to process
//
//  INPUTS:
//
//      Queue    - Our default queue
//
//      Request  - A write request
//
//      Length   - The length of the write operation
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//  IRQL:
//
//      This routine is called at IRQL <= DISPATCH_LEVEL
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
VOID
NothingEvtWrite(WDFQUEUE   Queue,
                WDFREQUEST Request,
                size_t     Length)
{
    PNOTHING_DEVICE_CONTEXT devContext;
    NTSTATUS                status;
    WDFREQUEST              readRequest;
    PVOID                   writeBuffer;
    size_t                  writeBufferLen;
    PVOID                   readBuffer;
    size_t                  readBufferLen;
    size_t                  copyLen;
    WDFFILEOBJECT           fileObject;

    UNREFERENCED_PARAMETER(Length);

#if DBG
    DbgPrint("NothingEvtRead\n");
#endif

    //
    // Get a pointer to our device context.
    // (get the WDFDEVICE from the WDFQUEUE, and the extension from the device)
    //
    devContext = NothingGetContextFromDevice(
                                             WdfIoQueueGetDevice(Queue));


    //
    // Check to see if this caller is THE open instances that
    // we'll allowed to write to the device.
    //
    // First, get the WDFFILEOBJECT handle that was used to send this Request.
    //
    fileObject = WdfRequestGetFileObject(Request);

    //
    // Was this on the allowed open instance??
    //
    if (devContext->AllowedWriter != fileObject) {

        //
        // Nope.  Block the write.  Return an error.
        //
#if DBG
        DbgPrint("File object 0x%p not allowed to write!\n",
                 fileObject);
#endif
        status = STATUS_MEDIA_WRITE_PROTECTED;

        copyLen = 0;

        goto DoneCompleteWrite;
    }

    //
    // We have received a WRITE.  Are there any READS waiting?
    //
    status = WdfIoQueueRetrieveNextRequest(devContext->ReadQueue,
                                           &readRequest);

    if (!NT_SUCCESS(status)) {

#if DBG
        DbgPrint("Failed to retrieve from the read queue (0x%x). "
                 "Queuing read\n",
                 status);
#endif

        //
        // There are no reads waiting.
        //
        // Put the write Request we just received onto the write Queue and return
        // with that write Request pending.
        //
        status = WdfRequestForwardToIoQueue(Request,
                                            devContext->WriteQueue);


        if (!NT_SUCCESS(status)) {

            //
            // Wow... we were unable to put the write Request onto the manual write Queue.
            // Not sure how we could get an error here, but... 
            //
#if DBG
            DbgPrint("WdfRequestForwardToIoQueue failed with Status code 0x%x",
                     status);
#endif
            copyLen = 0;

            goto DoneCompleteWrite;
        }

        goto DoneJustReturn;
    }

    //
    // We DO have a read! Satisfy the write we just received using the
    // data from the read that we just removed from the Queue.
    //
#if DBG
    DbgPrint("read request was pending!\n");
#endif

    //
    // Get the read buffer...
    //
    status = WdfRequestRetrieveOutputBuffer(readRequest,
                                            1,
                                            &readBuffer,
                                            &readBufferLen);
    if (!NT_SUCCESS(status)) {
#if DBG
        DbgPrint("WdfRequestRetrieveOutputBuffer failed with Status code 0x%x",
                 status);
#endif

        //
        // Bad buffer... Fail both requests.
        //

        copyLen = 0;

        goto DoneCompleteBoth;
    }

    //
    // Get the write buffer...
    //
    status = WdfRequestRetrieveInputBuffer(Request,
                                           1,
                                           &writeBuffer,
                                           &writeBufferLen);
    if (!NT_SUCCESS(status)) {

#if DBG
        DbgPrint("WdfRequestRetrieveInputBuffer failed with Status code 0x%x",
                 status);
#endif
        //
        // Sigh... Fail both requests.
        //

        copyLen = 0;

        goto DoneCompleteBoth;
    }

    //
    // Only copy as much as the smaller of the two buffers. Sure, we
    // "lose" some data, but good enough for this exercise.
    //
    copyLen = min(writeBufferLen,
                  readBufferLen);

    //
    // Store the data from the write into the user's read buffer
    //
    RtlCopyMemory(readBuffer,
                  writeBuffer,
                  copyLen);

DoneCompleteBoth:

#if DBG
    DbgPrint("Completing requests with 0x%Ix bytes transferred\n",
             copyLen);
#endif

    //
    // Done with the read
    //
    WdfRequestCompleteWithInformation(readRequest,
                                      status,
                                      copyLen);
DoneCompleteWrite:

    //
    // Done with the write
    //
    WdfRequestCompleteWithInformation(Request,
                                      status,
                                      copyLen);
DoneJustReturn:

    return;
}

///////////////////////////////////////////////////////////////////////////////
//
//  NothingEvtDeviceControl
//
//    This routine is called by the framework when there is a
//    device control request for us to process
//
//  INPUTS:
//
//      Queue    - Our default queue
//
//      Request  - A device control request
//
//      OutputBufferLength - The length of the output buffer
//
//      InputBufferLength  - The length of the input buffer
//
//      IoControlCode      - The operation being performed
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//  IRQL:
//
//      This routine is called at IRQL == PASSIVE_LEVEL, due to
//      our PASSIVE_LEVEL execution level contraint
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
VOID
NothingEvtDeviceControl(WDFQUEUE   Queue,
                        WDFREQUEST Request,
                        size_t     OutputBufferLength,
                        size_t     InputBufferLength,
                        ULONG      IoControlCode)
{
    UNREFERENCED_PARAMETER(IoControlCode);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(Queue);

#if DBG
    DbgPrint("NothingEvtDeviceControl\n");
#endif

    //
    // Nothing to do...
    // In this case, we return an info field of zero
    //
    WdfRequestCompleteWithInformation(Request,
                                      STATUS_SUCCESS,
                                      0);
}

///////////////////////////////////////////////////////////////////////////////
//
//  NothingEvtFdoCreate
//
//    This routine is called by the framework when someone is
//    trying to open a HANDLE to our device
//
//  INPUTS:
//
//      Device   - One of our devices
//
//      Request  - A create request
//
//      FileObject - The WDFFILEOBJECT representing the new
//                   open instance
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//  IRQL:
//
//      This routine is called at IRQL == PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
VOID
NothingEvtFdoCreate(
    WDFDEVICE     Device,
    WDFREQUEST    Request,
    WDFFILEOBJECT FileObject)
{
    PNOTHING_DEVICE_CONTEXT devContext;
    NTSTATUS                status;

    devContext = NothingGetContextFromDevice(Device);

    WdfSpinLockAcquire(devContext->FileObjectsCollectionLock);

    //
    // Check to see if we don't already have an "allowed writer"
    //
    if (devContext->AllowedWriter == nullptr) {
#if DBG
        DbgPrint("First one in. 0x%p allowed to write\n",
                 FileObject);
#endif

        //
        // Allow this caller to write to the device
        //
        status                    = STATUS_SUCCESS;
        devContext->AllowedWriter = FileObject;

    } else {

#if DBG
        DbgPrint("Adding 0x%p to collection\n",
                 FileObject);
#endif
        //
        // Put this caller on the "waiting to be allowed to write" list
        //
        status = WdfCollectionAdd(devContext->FileObjectsCollection,
                                  FileObject);

        if (!NT_SUCCESS(status)) {
#if DBG
            DbgPrint("WdfCollectionAdd failed 0x%0x\n",
                     status);
#endif
        }

    }

    WdfSpinLockRelease(devContext->FileObjectsCollectionLock);

    //
    // Done with this request
    //
    WdfRequestCompleteWithInformation(Request,
                                      status,
                                      0);
}


///////////////////////////////////////////////////////////////////////////////
//
//  NothingEvtFdoClose
//
//    This routine is called by the framework when a file object
//    that represents an open instance of our device is going
//    away
//
//  INPUTS:
//
//      FileObject - The WDFFILEOBJECT representing the closing
//                   open instance
//
//  OUTPUTS:
//
//      None.
//
//  RETURNS:
//
//      None.
//
//  IRQL:
//
//      This routine is called at IRQL == PASSIVE_LEVEL.
//
//  NOTES:
//
//
///////////////////////////////////////////////////////////////////////////////
VOID
NothingEvtFdoClose(WDFFILEOBJECT FileObject)
{
    PNOTHING_DEVICE_CONTEXT devContext;
    WDFFILEOBJECT           newWriter;

    devContext =
            NothingGetContextFromDevice(WdfFileObjectGetDevice(FileObject));


    WdfSpinLockAcquire(devContext->FileObjectsCollectionLock);

    //
    // Was this open the allowed writer?
    //
    if (devContext->AllowedWriter == FileObject) {
#if DBG
        DbgPrint("Writer 0x%p closing\n",
                 FileObject);
#endif

        //
        // The allowed writer is going away. Try to promote the next
        // person in line to being the writer.
        //
        newWriter = (WDFFILEOBJECT)WdfCollectionGetFirstItem(devContext->FileObjectsCollection);

        if (newWriter) {

#if DBG
            DbgPrint("0x%p is now allowed to write\n",
                     newWriter);
#endif
            //
            // Remove him from the collection.
            //
            WdfCollectionRemove(devContext->FileObjectsCollection,
                                newWriter);

        } else {
#if DBG
            DbgPrint("No one else waiting to be a writer\n");
#endif
        }

        devContext->AllowedWriter = newWriter;

    } else {

        //
        // Someone going away before they had a chance to write. Just take
        // them out of the list.
        //
#if DBG
        DbgPrint("Removing non-writer 0x%p from the collection\n",
                 FileObject);
#endif
        WdfCollectionRemove(devContext->FileObjectsCollection,
                            FileObject);
    }

    WdfSpinLockRelease(devContext->FileObjectsCollectionLock);
}


CHAR const*
NothingPowerDeviceStateToString(
    WDF_POWER_DEVICE_STATE DeviceState
    )
{
    switch (DeviceState) {
        case WdfPowerDeviceInvalid:
            return "WdfPowerDeviceInvalid";
        case WdfPowerDeviceD0:
            return "WdfPowerDeviceD0";
        case WdfPowerDeviceD1:
            return "WdfPowerDeviceD1";
        case WdfPowerDeviceD2:
            return "WdfPowerDeviceD2";
        case WdfPowerDeviceD3:
            return "WdfPowerDeviceD3";
        case WdfPowerDeviceD3Final:
            return "WdfPowerDeviceD3Final";
        case WdfPowerDevicePrepareForHibernation:
            return "WdfPowerDevicePrepareForHibernation";
        case WdfPowerDeviceMaximum:
            return "WdfPowerDeviceMaximum";
        default:
            break;
    }
    return "Unknown";
}
