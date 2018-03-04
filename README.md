# Weather_station-mqtt-using-ubidots-
It is a portable weather station once the code is uploaded to NodeMcu we don't have to modify it for connecting to any
other network. Just have to press the button connected to pin D3 and a AP open for configuring the esp8266 for connecting 
to the network.

The code is written in arduino IDE and uses PubSub client to send the data of DHT11 sensor to the server.
The code is written for esp8266 and also host a local server to send data on a html webpage the temperature and humidity.
The code includes wifi manager library to set the trigger pin (D3) for the Acess Point
