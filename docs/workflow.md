
## Board initialization workflow

The functions:

- EarlyInit
- CriticalInit
- BoardInit
- PostInit

must be overridden in a derived class. Those are called by the `Initialize` function.

The `Initialize` function will, in order:

- call esp32hal::CPU cpu.ReadChipInfo
- call `EarlyInit` and if that fails exits with severity level set to **5**.
- call `PowerPeripherals(true)` and if that fails exits with severity level set to **5**.
- initialize NVS and if that fails exits with severity level set to **5**.
- read configuration from NVS. If the stored configuration is not valid it will build an empty, default, one.
- create the default event loop and if that fails exits with severity level set to **5**.
- create the event handler for WiFiManager and if that fails exits with severity level set to **5**.
- initialize the underlying TCP/IP stack and if that fails exits with severity level set to **5**.
- call `CriticalInit` and if that fails exits with severity level set to **5**.
- call `BoardInit` and if that fails exits with severity level set to **2**.
- set the power mode to `WIFI_PS_NONE` otherwise the board will not respond to ARP requests.
- set the board information like application name and version, cpu and memory type, etc.
- call `PostInit` and if that fails exits with severity level set to **1**.

### EarlyInit

Is meant to be used for initializing known GPIO and peripherals.

### CriticalInit

Is meant to initialize components required for device's basic functionality, like a LoRa module.

### BoardInit

To initialize optional components.

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
