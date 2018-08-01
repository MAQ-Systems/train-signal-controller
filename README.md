# Train Signal Controller
### Directory Structure:
- __SignalConsumer__: Arduino software that reads signals from the train signal web API.
    - This can be modified and flashed to the device with the [Arduino IDE](https://www.arduino.cc/en/Main/Software).
- __TrainSignalApi__: A Java servlet application that runs on the server. This software is responsible for both serving the web API and listening for connections from the SignalConsumer.
    - This is an [Eclipse Java EE](https://www.eclipse.org/downloads/) project.
    - Alternatively, this can be built using [Apache Ant](https://ant.apache.org/bindownload.cgi) with [Ivy](http://ant.apache.org/ivy/download.cgi) using the build.xml file in this directory.
        - The generated war file can be found in the "out" directory.
- __LocalTrainCTC__: An Android phone app to send color change commands to the signal.
    - This is an [Android Studio](https://developer.android.com/studio/) project.
- __assets__: Images used for the project.

### Architecture:
The TrainSignalApi is always listening for connections from the SignalConsumer. If the connection is interrupted, the SignalConsumer will attempt to establish a new connection. The server is only capable of hosting a single connection to a signal, so when a new connection is requested (due to interruption or otherwise), the existing connection will be dropped to host the new one. The SignalConsumer has a delay of 5 seconds after receiving a command before processing the next, giving it time to physically change.

### API:
TrainSignalApi/api?color=__COLOR__&lamp=__LAMPSTATE__

- COLOR: One of the following:
    - r: Change the signal to red.
    - y: Change the signal to yellow.
    - g: Change the signal to green.

- LAMPSTATE: One of the following:
    - 0: Turn the lamp off.
    - b: Blink the lamp (this needs to be implemented in hardware).
    - 1: Turn the lamp on.

### Hardware:

- __Arduino UNO__: The microcontroller that lives in the train signal.
- __Arduino ethernet shield__: Used to receive commands from the network. 
- __Power supply__: 12V and at least 3A depending on number of lamps.
- __Relay board__: Used to switch larger power supply.