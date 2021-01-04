
## Board initialization workflow

The functions:

- EarlyInit
- CriticalInit
- BoardInit
- PostInit

must be overridden in a derived class. Those are called by the `Initialize` function.

The `Initialize` function will, in order:

- call esp32hal::CPU cpu.ReadChipInfo
- call `EarlyInit` and call `GoodBye` if that fails.
- call `PowerPeripherals(true)` and call `GoodBye` if that fails.
- initialize NVS and call `GoodBye` if that fails.
- read configuration from NVS. If the stored configuration is not valid it will build an empty, default, one.
- create the default event loop and call `GoodBye` if that fails.
- create the event handler for WiFiManager and call `GoodBye` if that fails.
- initialize the underlying TCP/IP stack and call `GoodBye` if that fails.
- call `CriticalInit` and call `GoodBye` if that fails.
- call `BoardInit`. If this fails, `Initialize` will exit.
- set the power mode to `WIFI_PS_NONE` otherwise the board will not respond to ARP requests.
- set the board information like application name and version, cpu and memory type, etc.
- call `PostInit`

### EarlyInit

Is meant to be used for initializing known GPIO and peripherals.

If this function fails, `Initialize` will call `GoodBye`.

### CriticalInit

Is meant to initialize components required for device's basic functionality, like a LoRa module.

If this function fails, `Initialize` will call `GoodBye`.

### BoardInit

To initialize optional components. If this function fails a log message is generated and the workflow will be interrupted.

### PostInit

Everything should be initialized before this function is called.

By overriding this function you can do the final tasks before main program starts, like connecting to an AP in station mode.

## Reconnection

Check the board connection status by calling `IsConnectedToAP` function.

To reconnect you should:

- stop any server (HTTP, HTTTPS, MQTT, etc.)
- call `RestartStationMode`
- if `RestartStationMode` returns ESP_OK:
  - start the servers (HTTP, HTTTPS, MQTT, etc.)
  - configure mDNS
