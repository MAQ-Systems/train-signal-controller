# Train Signal Controller
### Directory Structure:
- __./3d__: 3D printable models to replace existing (e.g. better suited) or
        missing parts.
- __./arduino__: Per-signal Arduino software that reads signals from the train
        signal web API.
  - Compile and flash to the device using [Arduino's IDE][arduino_link].
- __./assets__: Images used for the project.
- __./LocalTrainCTC__: __`[OUT OF DATE]`__ An Android phone app to send state
        change commands to the signal.
    - This is an [Android Studio][android_studio_link] project.
- __./photos__: Before/after photos of specific signals.
- __./TrainSignalApi__: A Java servlet application that runs on the server. This
        software is responsible for both serving the web API and listening for
        connections from the SignalConsumer.
    - This can be built using [Apache Ant][apache_ant_link] with
            [Ivy][apache_ivy_link] using the build.xml file in this directory.
            The generated `.war` file can be found in the "out" directory.
- __./web__: A simple web application to interact with the signal. This interface
        is preferred over the Android app.

### Architecture:
The TrainSignalApi is always listening for connections from the SignalConsumer.
If the connection is interrupted, the SignalConsumer will attempt to establish a
new connection. The server is only capable of hosting a single connection to a
signal, so when a new connection is requested (due to interruption or
otherwise), the existing connection will be dropped to host the new one. The
SignalConsumer has a delay of 2.5 seconds after receiving a command before
processing the next, giving it time to physically change.

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

[arduino_link]:https://www.arduino.cc/en/software
[android_studio_link]:https://developer.android.com/studio/
[apache_ant_link]:https://ant.apache.org
[apache_ivy_link]:http://ant.apache.org/ivy