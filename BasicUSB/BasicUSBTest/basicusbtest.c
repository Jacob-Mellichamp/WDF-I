//
// basicusbtest.c
//
// Win32 console mode program to fire up requests to the
// BASICUSB driver.
//
// This code is purely functional, and is definitely not designed to be any
// sort of example.
//
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <windows.h>
#include <basicusb_ioctl.h>

//
// Simple test application to demonstrate the BasicUSB driver
//

int
__cdecl
main(ULONG argc, LPSTR *argv)
{
    HANDLE deviceHandle;
    DWORD  code;
    ULONG  index;
    UCHAR  writeBuffer[512];
    UCHAR  readBuffer[512];
    DWORD  function;
    UCHAR  barGraph;

    //
    // Init the write and read buffers with known data
    //
    memset(readBuffer, 0xAA, sizeof(readBuffer));

    memset(writeBuffer, 0xEE, sizeof(writeBuffer));

    //
    // Open the sample PCI device
    //
    deviceHandle = CreateFile(L"\\\\.\\BASICUSB",
                              GENERIC_READ|GENERIC_WRITE,
                              0,
                              0,
                              OPEN_EXISTING,
                              0,
                              0);

    //
    // If this call fails, check to figure out what the error is and report it.
    //
    if (deviceHandle == INVALID_HANDLE_VALUE) {

        code = GetLastError();

        printf("CreateFile failed with error 0x%lx\n", code);

        return(code);
    }

    //
    // Infinitely print out the list of choices, ask for input, process
    // the request
    //
    while(TRUE)  {

        printf ("\nBASICUSB TEST -- functions:\n\n");
        printf ("\t1. Send a sync READ\n");
        printf ("\t2. Send a sync WRITE\n");
        printf ("\t3. Send SET_BAR_GRAPH IOCTL - All On\n");
        printf ("\t4. Send SET_BAR_GRAPH IOCTL - All Off\n");
        printf ("\n\t0. Exit\n");
        printf ("\n\tSelection: ");
#pragma warning(suppress: 6031)
        scanf ("%x", &function);  // NOLINT(cert-err34-c)

        switch(function)  {

        case 1:
            //
            // Send a read
            //
            if ( !ReadFile(deviceHandle,
                           readBuffer,
                           sizeof(readBuffer),
                           &index,
                           NULL)) {

                code = GetLastError();

                printf("ReadFile failed with error 0x%lx\n", code);

                return(code);
            }
            printf("Bytes Read = %ld.\n", index);
            break;

        case 2:
            //
            // Send a write
            //
            if (!WriteFile(deviceHandle,
                           writeBuffer,
                           sizeof(writeBuffer),
                           &index, 
                           NULL)) {

                code = GetLastError();

                printf("WriteFile failed with error 0x%lx\n", code);

                return(code);
            }
            printf("Bytes Written = %ld.\n", index);
            break;

        case 3:
            barGraph = 0xFF;

            if (!DeviceIoControl(deviceHandle,
                                 IOCTL_OSR_BASICUSB_SET_BAR_GRAPH,
                                 &barGraph,
                                 sizeof(UCHAR),
                                 NULL,
                                 0,
                                 &index,
                                 NULL)) {

                code = GetLastError();

                printf("DeviceIoControl failed with error 0x%lx\n", code);
                return(code);

            }

            printf("IOCTL worked!\n");
            break;
        case 4:
            barGraph = 0;

            if (!DeviceIoControl(deviceHandle,
                                 IOCTL_OSR_BASICUSB_SET_BAR_GRAPH,
                                 &barGraph,
                                 sizeof(UCHAR),
                                 NULL,
                                 0,
                                 &index,
                                 NULL)) {

                code = GetLastError();

                printf("DeviceIoControl failed with error 0x%lx\n", code);
                return(code);

            }
            printf("IOCTL worked!\n");
            break;
        case 0:

            //
            // zero is get out!
            //
            return(0);

        default:
            break;

        }
    }   
}
