/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _SDL_peripheral_h
#define _SDL_peripheral_h

/**
 *  \file SDL_third.h
 *
 *  Header for the SDL third party management routines.
 */

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_video.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

//
// misc card. Second IDCard of China etc.
//
typedef enum {
	// GT/T 2261.1--2003
	SDL_GenderUnknown,
	SDL_GenderMale,
	SDL_GenderFemale,
} SDL_Gender;

typedef enum {
	SDL_CardIDNone,
	SDL_CardIDCard,
	SDL_CardIC34,
	SDL_CardIC26,
	SDL_CardID,
	SDL_CardRFID
} SDL_CardType;

typedef struct {
	SDL_CardType type;
	char name[48];
	SDL_Gender gender;
	int folk;
	char birthday[12]; // yyyy-mm-dd
	char address[128];
	char number[48];
	SDL_Surface* photo;
	SDL_Surface* photo2;
} SDL_IDCardInfo;

extern DECLSPEC SDL_bool SDL_ReadCard(SDL_IDCardInfo* info);

//
// serial port
//
#ifdef _WIN32
typedef HANDLE	SDL_handle;
#define SDL_INVALID_HANDLE_VALUE	INVALID_HANDLE_VALUE

#else
typedef int		SDL_handle;
#define SDL_INVALID_HANDLE_VALUE	-1
#endif

extern DECLSPEC SDL_handle SDL_OpenSerialPort(const char* path, int baudrate);
extern DECLSPEC void SDL_CloseSerialPort(SDL_handle h);
extern DECLSPEC size_t SDL_ReadSerialPort(SDL_handle h, void* ptr, size_t size);
extern DECLSPEC size_t SDL_WriteSerialPort(SDL_handle h, const void* ptr, size_t size);

//
// system control
//
extern DECLSPEC SDL_bool SDL_SetTime(int64_t utctime);
extern DECLSPEC void SDL_UpdateApp(const char* package);

typedef struct {
	char manufacturer[40];
	char model[40];
	char display[128];
} SDL_OsInfo;
extern DECLSPEC void SDL_GetOsInfo(SDL_OsInfo* info);
extern DECLSPEC SDL_bool SDL_PrintText(const uint8_t* bytes, int size);

/**
*  \brief Enable/Disable Relay.
*/
extern DECLSPEC void SDLCALL SDL_EnableRelay(SDL_bool enable);

typedef enum {
	SDL_LColorNone,
	SDL_LColorWhite,
	SDL_LColorRed,
	SDL_LColorGreen,
	SDL_LColorBlue,
	SDL_LColorBlack,
} SDL_LightColor;
extern DECLSPEC void SDLCALL SDL_TurnOnFlashlight(SDL_LightColor color);
extern DECLSPEC void SDLCALL SDL_SetBrightness(int brightness);

//
// wifi
//
typedef enum {
	SDL_WifiFlagEnable = 0x1,
	SDL_WifiFlagScanResultChanged = 0x2,
} SDL_WifiFlag;


#define SDL_WIFI_MAX_SSID_BYTES	63
typedef enum {
	SDL_WifiApFlagConnected = 0x1,
} SDL_WifiApFlag;

typedef struct {
	char ssid[SDL_WIFI_MAX_SSID_BYTES + 1];
	// The detected signal level in dBm, also known as the RSSI.
	int rssi;
	uint32_t flags;
} SDL_WifiScanResult;

extern DECLSPEC void SDL_WifiSetEnable(SDL_bool enable);
extern DECLSPEC uint32_t SDL_WifiGetFlags(void);
extern DECLSPEC int SDL_WifiGetScanResults(SDL_WifiScanResult** results);
extern DECLSPEC SDL_bool SDL_WifiConnect(const char* ssid, const char* password);
extern DECLSPEC SDL_bool SDL_WifiRemove(const char* ssid);

//
// bluetooth
//
#define SDL_BLE_MAC_ADDR_BYTES	6

typedef enum {
	SDL_BleCharacteristicPropertyBroadcast = 0x01,
	SDL_BleCharacteristicPropertyRead = 0x02,
	SDL_BleCharacteristicPropertyWriteWithoutResponse = 0x04,
	SDL_BleCharacteristicPropertyWrite = 0x08,
	SDL_BleCharacteristicPropertyNotify = 0x10,
	SDL_BleCharacteristicPropertyIndicate = 0x20,
	SDL_BleCharacteristicPropertyAuthenticatedSignedWrites = 0x40,
	SDL_BleCharacteristicPropertyExtendedProperties = 0x80,
	SDL_BleCharacteristicPropertyNotifyEncryptionRequired = 0x100,
	SDL_BleCharacteristicPropertyIndicateEncryptionRequired = 0x200,
} SDL_BleCharacteristicProperties;

struct _SDL_BleService;

typedef struct {
	char* uuid;
	struct _SDL_BleService* service;
	uint32_t properties;
	void* cookie;
} SDL_BleCharacteristic;

typedef struct _SDL_BleService {
	char* uuid;
	SDL_BleCharacteristic* characteristics;
	int valid_characteristics;
	void* cookie;
} SDL_BleService;

typedef struct {
	char* name;
	char* uuid;
	uint8_t* manufacturer_data;
	int manufacturer_data_len;
	int rssi;
    uint8_t mac_addr[SDL_BLE_MAC_ADDR_BYTES];
	SDL_BleService* services;
	int valid_services;
   
    unsigned int receive_advertisement;
	void* cookie;
} SDL_BlePeripheral;

typedef struct {
    void (*discover_peripheral)(SDL_BlePeripheral* peripheral);
    void (*release_peripheral)(SDL_BlePeripheral* peripheral);
    void (*connect_peripheral)(SDL_BlePeripheral* peripheral, int error);
    void (*disconnect_peripheral)(SDL_BlePeripheral* peripheral, int error);
	void (*discover_services)(SDL_BlePeripheral* peripheral, int error);
	void (*discover_characteristics)(SDL_BlePeripheral* peripheral, SDL_BleService* service, int error);
    void (*read_characteristic)(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic, const uint8_t* data, int len);
    void (*write_characteristic)(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic, int error);
    void (*notify_characteristic)(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic, int error);
	void (*discover_descriptors)(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic, int error);
} SDL_BleCallbacks;

extern DECLSPEC SDL_BleService* SDL_BleFindService(SDL_BlePeripheral* peripheral, const char* uuid);
extern DECLSPEC SDL_BleCharacteristic* SDL_BleFindCharacteristic(SDL_BlePeripheral* peripheral, const char* service_uuid, const char* uuid);

extern DECLSPEC void SDL_BleSetCallbacks(SDL_BleCallbacks* callbacks);
extern DECLSPEC void SDL_BleScanPeripherals(const char* uuid);
extern DECLSPEC void SDL_BleStopScanPeripherals(void);
extern DECLSPEC void SDL_BleReleasePeripheral(SDL_BlePeripheral* peripheral);
extern DECLSPEC void SDL_BleStartAdvertise(void);
extern DECLSPEC void SDL_BleConnectPeripheral(SDL_BlePeripheral* peripheral);
extern DECLSPEC void SDL_BleDisconnectPeripheral(SDL_BlePeripheral* peripheral);
extern DECLSPEC void SDL_BleGetServices(SDL_BlePeripheral* peripheral);
extern DECLSPEC void SDL_BleClearCache(SDL_BlePeripheral* peripheral);
extern DECLSPEC void SDL_BleGetCharacteristics(SDL_BlePeripheral* peripheral, SDL_BleService* service);
extern DECLSPEC void SDL_BleReadCharacteristic(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic);
extern DECLSPEC void SDL_BleWriteCharacteristic(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic, const uint8_t* data, int size);
extern DECLSPEC void SDL_BleNotifyCharacteristic(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic, SDL_bool enable);
extern DECLSPEC void SDL_BleDiscoverDescriptors(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic);
extern DECLSPEC int SDL_BleAuthorizationStatus(void);
extern DECLSPEC SDL_bool SDL_BleUuidEqual(const char* uuid1, const char* uuid2);

// use as peripheral special
typedef struct {
	void (*advertising_started)(int error);
	void (*center_connected)(const char* name, uint8_t* macaddr, int error);
	void (*center_disconnected)(void);
	void (*read_characteristic)(const char* characteristic, const uint8_t* data, int len);
    void (*write_characteristic)(const char* characteristic, int error);
	void (*notify_characteristic)(const char* characteristic, int error);
	// peripheral had written bytes to a notification char, and these bytes has written.
	void (*notification_sent)(const char* characteristic, int error);
} SDL_PBleCallbacks;

extern DECLSPEC void SDL_PBleSetCallbacks(SDL_PBleCallbacks* callbacks);
extern DECLSPEC void SDL_PBleStartAdvertising(const char* name, int manufacturer_id, const uint8_t* manufacturer_data, int manufacturer_data_size);
extern DECLSPEC void SDL_PBleStopAdvertising(void);
extern DECLSPEC SDL_bool SDL_PBleIsAdvertising(void);
extern DECLSPEC void SDL_PBleWriteCharacteristic(const char* chara_uuid, const uint8_t* data, int size);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_timer_h */

/* vi: set ts=4 sw=4 expandtab: */
