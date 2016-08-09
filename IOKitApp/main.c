//
//  main.c
//  IOKitApp
//
//  Created by Lamb, Christopher Charles on 8/9/16.
//  Copyright © 2016 Lamb, Christopher Charles. All rights reserved.
//

#include <stdio.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#define SERVICE_NAME "IOUSBDevice"

typedef struct {
    io_service_t service;
    io_object_t notification;
} MyDriverData;

IONotificationPortRef gNotificationPort = NULL;

void deviceNotification(void* refCon,
                        io_service_t service,
                        natural_t messageType,
                        void* messageArgument) {
    MyDriverData* myDriverData = (MyDriverData*) refCon;
    if (messageType == kIOMessageServiceIsTerminated) {
        io_name_t   name;
        IORegistryEntryGetName(service, name);
        printf("Device %s removed.\n", name);
        IOObjectRelease(myDriverData->notification);
        IOObjectRelease(myDriverData->service);
        free(myDriverData);
    }
}

void deviceAdded(void* refCon, io_iterator_t iter) {
    io_service_t service = 0;
    while ((service = IOIteratorNext(iter)) != 0) {
        MyDriverData* myDriverData
            = (MyDriverData*) malloc(sizeof(MyDriverData));
        myDriverData->service = service;
        
        CFStringRef className = IOObjectCopyClass(service);
        if (CFEqual(className, CFSTR(SERVICE_NAME)) == true) {
            io_name_t name;
            IORegistryEntryGetName(service, name);
            printf("Found: %s\n", name);
        }
        CFRelease(className);
        
        kern_return_t kr = IOServiceAddInterestNotification(gNotificationPort,
                                                            service,
                                                            kIOGeneralInterest,
                                                            deviceNotification,
                                                            myDriverData,
                                                            &myDriverData->notification);
        if (kr != KERN_SUCCESS) {
            free(myDriverData);
            exit(1);
        }
    }
}

// ***
// This group of functions enumerates USB devices via callback.
// ***
void deviceAddedEnumerateName(void* refCon, io_iterator_t iter) {
    io_service_t service = 0;
    while((service = IOIteratorNext(iter)) != 0) {
        CFStringRef className = IOObjectCopyClass(service);
        if (CFEqual(className, CFSTR(SERVICE_NAME)) == true) {
            io_name_t name;
            IORegistryEntryGetName(service, name);
            printf("Found: %s\n", name);
        }
        CFRelease(className);
        IOObjectRelease(service);
    }
}

int runNotification(void) {
    CFDictionaryRef matchingDict = IOServiceMatching(SERVICE_NAME);
    
    gNotificationPort = IONotificationPortCreate(kIOMasterPortDefault);
    CFRunLoopSourceRef runLoopSource
        = IONotificationPortGetRunLoopSource(gNotificationPort);
    CFRunLoopAddSource(CFRunLoopGetCurrent(),
                       runLoopSource,
                       kCFRunLoopDefaultMode);
    
    
    io_iterator_t iter = 0;
    kern_return_t kr = IOServiceAddMatchingNotification(gNotificationPort,
                                                        kIOFirstMatchNotification,
                                                        matchingDict,
                                                        deviceAdded,
                                                        NULL,
                                                        &iter);
    if (kr != KERN_SUCCESS) {
        return 1;
    }
    
    deviceAdded(NULL, iter);
    
    CFRunLoopRun();
    
    IONotificationPortDestroy(gNotificationPort);
    IOObjectRelease(iter);
    
    return 0;
}



int main(int argc, const char * argv[]) {
    return runNotification();
}
