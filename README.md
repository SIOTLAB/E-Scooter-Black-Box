<h1 align="center"> E-Scooter Rough Riding Detection

  ![Python](https://img.shields.io/badge/python-v3.8+-blue.svg)
  ![C](https://img.shields.io/badge/gcc-v14.38+-pink.svg)
  ![Dependencies](https://img.shields.io/badge/dependencies-up%20to%20date-brightgreen.svg)
  [![GitHub Issues](https://img.shields.io/github/issues/joshuajerome/senior-des.svg)](https://github.com/joshuajerome/senior-des/issues)
  ![Contributions welcome](https://img.shields.io/badge/contributions-welcome-yellow.svg)
  
</h1>

## About

As cities become increasingly congested, ride sharing has become an appealing alternative to traditional modes of transportation. Bicycle and scooter sharing are some of the most popular forms of ride sharing, especially in settings where most customers are looking to travel short distances for personal trips. As with all mobile transporation, cost and security are important factors to consider during usage. Furthermore, it is important to identify users or circumstances that compromise the functionality/safety of the scooter or person respectively. We propose a solution involving a ‘black box’ system that will be able to detect misuse and rough handling of e-bicycles and e-scooters in order to improve the cost-effectiveness and appeal of ride sharing services.

## Usage

First clone this repository, then follow the steps listed under _Sensortile.box Pro Programming_ [here](https://github.com/joshuajerome/senior-des#sensortilebox-pro-programming)

```shell
git clone https://github.com/joshuajerome/senior-des.git
```

## Scripts

The 'data_formatting' folder contains all Python scripts related to data processing and analysis of sensory data.

## Sensortile.box Pro Programming

<p align="center">
  <img src="https://github.com/joshuajerome/senior-des/assets/73908842/7dcb6000-f632-4019-b06c-a8dfb3e82adb">
</p>

> The STMDataPack folder contains all of the files and resources necessary to program the SensorTileBox Pro. **_Ensure a proper installation of [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) and [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html) from ST's website prior to following the steps below_** to initialize a project to program the SensorTileBox Pro.

### Initializing the Sensortile.box Pro Project

1. Open STM32CubeIDE. In the top menu, select _File -> Open Project From File System_.

###
2. In the resulting prompt, select the 'Applications' folder, located inside the STMDataPack, after clicking on the 'Directory' button. Additionally, **make sure that the options are selected as shown in the image below** (the checkbox for the project 'BLEPiano' shouldn't be grayed out – unlike in the image – if you are importing the project for the first time).
<img width="700" alt="Screenshot 2024-02-20 at 2 46 20 PM" src="https://github.com/joshuajerome/senior-des/assets/50118626/cb7d7861-687e-4a7f-87ec-c61d78a3f9f1">
<img width="1041" alt="Screenshot 2024-02-20 at 2 48 12 PM" src="https://github.com/joshuajerome/senior-des/assets/50118626/7a6129df-616f-47de-ace4-0dd250a90c45">

###
3. Once the project has been imported, you may make any changes as necessary (most of the code is in the _Application/User/main.c_ file). Once you're ready to run the code, select the project file icon in the left sidebar and navigate to the properties panel. Then navigate to _C/C++ Builds -> Settings -> MCU Post-Build Outputs_ and ensure that the 'convert to binary file -o' option is enabled.
<img width="572" alt="Screenshot 2024-02-20 at 2 52 41 PM" src="https://github.com/joshuajerome/senior-des/assets/50118626/091cf285-faa9-464f-9807-9a9b58016b8e">
<img width="808" alt="Screenshot 2024-02-20 at 2 53 16 PM" src="https://github.com/joshuajerome/senior-des/assets/50118626/9cddf6bf-7d06-4d4b-a1d0-2fe65b452526">

###
4. Similar to the last step, select the project file icon in the left sidebar, but select the 'Clean Project' option (A). **Make sure this runs to completion by checking the build output in the IDE Console with _Window -> Show View -> Console_. Sometimes this doesn't work the first time you do it.** After the full project is cleaned, then from the same project menu select the 'Build Project' option (B) and ensure no errors from the build in the console.
<img width="428" alt="Screenshot 2024-02-20 at 2 59 10 PM" src="https://github.com/joshuajerome/senior-des/assets/50118626/fb5505bf-47c5-4002-b34d-f27dbcd2a629">

###
5. Once the project has built successfully, check the following folder directory for a binary file that should've been recently generated: _.../senior-des/STMDataPack/Projects/STM32U585AI-SensorTile.boxPro/Applications/BLEPiano/STM32CubeIDE/Debug/**BLEPiano.bin**_. We will use STM32CubeProgrammer to flash this same file onto the SensorTileBox Pro firmrware.
<img width="709" alt="Screenshot 2024-02-20 at 3 08 29 PM" src="https://github.com/joshuajerome/senior-des/assets/50118626/70d3d75b-f01c-4ad5-b2d0-5bc9f6025be4">

###
6. Plug the SensorTileBox Pro into your machine with a USB-C cable, and make sure to take off the top blue plastic cover by unscrewing the 4 screws on the outside of the SensorTileBox. Ensure the top of the SensorTileBox board is facing up, so you can see the LED's on the board.
<img width="878" alt="Screenshot 2024-02-20 at 3 18 51 PM" src="https://github.com/joshuajerome/senior-des/assets/50118626/fb54753c-4288-4d56-ab70-b42ba5709ee3">

###
7. Open STM32CubeProgrammer. Upon opening it, you should see a right sidebar as shown below. Select the options shown, and **also ensure the SensorTileBox power switch (on the side of the box) is 'off'**.
<img width="318" alt="Screenshot 2024-02-20 at 3 20 22 PM" src="https://github.com/joshuajerome/senior-des/assets/50118626/e49f9037-de95-489e-8e57-4c4eb1b71563">

###
8. In order to flash the project binary from Step 5 into the SensorTileBox, the Box needs to be booted up into DFU Mode. Hold down the button on the side of the box labeled '2' and simultaneously slide the power switch into the 'on' position. Once the box is on, let go of the '2' button. The box should show two flashing lights and nothing else – there should be no sounds or additional LED's flashing in DFU mode.
###
9. Once the Box is in DFU mode, select the 'connect' button in the CubeProgrammer interface from Step 8. There should be some console output that shows in the center and also the status icon in the top right should switch from 'disconnected' to 'connected'.
###
10. Navigate to the downward arrow icon on the narrow left sidebar. Then click on 'browse' to choose your binary file that was generated in Step 5 (an example filepath is shown in the second image below). Check the 'verify programming' option as well.
<img width="579" alt="Screenshot 2024-02-20 at 3 24 47 PM" src="https://github.com/joshuajerome/senior-des/assets/50118626/75d553bc-419e-4684-9ccc-828165ec6187">
<img width="1192" alt="Screenshot 2024-02-20 at 3 26 18 PM" src="https://github.com/joshuajerome/senior-des/assets/50118626/63988b48-1d76-4777-862f-debc49c5abfa">

###
11. Once the file has been selected, **ensure the board is connected to your machine and powered on (2 LED's should be on)**, and _then_ click on the start programming button. This should run to completion without any errors.
###
12. After CubeProgrammer has finished programming, turn the SensorTileBox off, and back on (keep it connected to the computer). You should hear a Piano sound, and in a few seconds your code from _main.c_ should start running.
