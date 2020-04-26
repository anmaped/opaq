
![opaq](data/www/images/opaq.png)

# IoT aquarium and pond ecosystem for everyone
#### Release 1.0.7

[![Join the chat at https://gitter.im/opaqproject/general](https://badges.gitter.im/opaqproject/general.svg)](https://gitter.im/opaqproject/general?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Main screen |  Settings Screen | Wifi Settings | Graph
:-----------:|:----------------:|:------:|:-------:
![](/data/binaries/screenshots/main.png) |  ![](/data/binaries/screenshots/settings.png) |  ![](/data/binaries/screenshots/wifi.png) |  ![](/data/binaries/screenshots/dimmergraph.png)

Supported features:
- Browser-based graphical user interface to control light and power devices of aquarium and pond systems
- Customizable programming actions for nodes such as automation of CO2 injection and temperature management
- Node-based architecture (nodes are authentic, secured with SHA256 signatures)
- Energy cost management that instruct you how to avoid wasting energy

Opaq devices available:

Opaq c1 | Opaq n1 | Opaq n2 | Opaq n3
:------:|:---------:|:---------:|:---------:
the main controller device ![Opaq C1 Frame](https://github.com/anmaped/opaq-hardware/blob/master/c1/v1.1/top.png?raw=true "Opaq C1 Frame") | the four channel dimmer device capable to connect with any led lamp (several settings available) ![Opaq n1 Frame](https://github.com/anmaped/opaq-hardware/blob/master/n1/v2.1/top.png?raw=true "Opaq n1 Frame") | the power outlet controller node for 220v devices up to 1400w ![Opaq n2 Frame](https://github.com/anmaped/opaq-hardware/blob/master/n2/top.png?raw=true "Opaq n2 Frame") | battery powered wireless sensor node ![Opaq n3 Frame](https://github.com/anmaped/opaq-hardware/blob/master/n3/top.png?raw=true "Opaq n3 Frame")

Expected features:
- Support for sensor logging (temperature, humidity, light intensity, PH, Nitrates(NO3))
- Management of peristaltic pumps for automatic fertilization regimes for fresh and salt water aquariums
- Dedicated settings for aquarium and pond systems (pond systems can be controlled by Opaq C1 to automatically perform programmable water changes, and feed fishes; pumps are controlled by programmable actions based on the information provided by water level sensor nodes)
- Dedicated non-introsive water management system based on opaq n2 battery powered 'mope' for sumps and Koi ponds
- Embedded UI

Expected Hardware:
- Opaq N3 (the node for probing PH, light intensity and humidity)
