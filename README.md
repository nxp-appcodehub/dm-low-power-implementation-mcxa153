# NXP Application Code Hub
[<img src="https://mcuxpresso.nxp.com/static/icon/nxp-logo-color.svg" width="100"/>](https://www.nxp.com)

## How to implement low power on MCXA153
The low power implementation demo is used for power mode switch, wake up time measurement and low power current measurement on the FRDM-MCXA153 board.

#### Boards: FRDM-MCXA153
#### Categories: Low Power, Power Conversion
#### Peripherals: CLOCKS, UART
#### Toolchains: MCUXpresso IDE

## Table of Contents
1. [Software](#step1)
2. [Hardware](#step2)
3. [Setup](#step3)
4. [Results](#step4)
5. [FAQs](#step5) 
6. [Support](#step6)
7. [Release Notes](#step7)

## 1. Software<a name="step1"></a>
- [MCUXpresso IDE V11.9.0 or later](https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools-/mcuxpresso-integrated-development-environment-ide:MCUXpresso-IDE)
- [SDK_2.14.0_FRDM-MCXA153](https://mcuxpresso.nxp.com/en/welcome)
- MCUXpresso for Visual Studio Code: This example supports MCUXpresso for Visual Studio Code, for more information about how to use Visual Studio Code please refer [here](https://www.nxp.com/design/training/getting-started-with-mcuxpresso-for-visual-studio-code:TIP-GETTING-STARTED-WITH-MCUXPRESSO-FOR-VS-CODE).

## 2. Hardware<a name="step2"></a>
- FRDM-MCXA153 Rev A board:

![FRDM-MCXA153](image/FRDM_MCXA153.png)

- One Type-C USB cable.

> If you want to measure wake up time, prepare an oscilloscope.

> If you want to measure low power current, prepare [MCU-Link Pro](https://www.nxp.com/design/design-center/software/development-software/mcuxpresso-software-and-tools-/mcu-link-pro-debug-probe:MCU-LINK-PRO) or Ammeter.

## 3. Setup<a name="step3"></a>

### 3.1 Hardware connection
- Use a Type-C USB cable to connect J15 of FRDM-MCXA153 and the USB port of the PC.

### 3.2 Import Project
1. Open MCUXpresso IDE 11.9.0, in the Quick Start Panel, choose **Import from Application Code Hub**

![import_from_ACH](image/import_from_ACH.png)


2. Enter the **demo name** in the search bar.

![ACH](image/ACH.JPG)

3. Click **Copy GitHub link**, MCUXpresso IDE will automatically retrieve project attributes, then click **Next>**.

![copy_github_link](image/copy_github_link.png)

4. Select **main** branch and then click **Next>**, select the MCUXpresso project, click **Finish** button to complete import.

> You need to install the [SDK_2.14.0_FRDM-MCXA153](https://mcuxpresso.nxp.com/en/welcome) on your MCUXpresso IDE.

### 3.3 Build and Flash Project
- Click **Build** button from the toolbar, then wait for the build to complete.

![MUCXpresso_Build](image/MCUXpresso_Build.png)

- Select the **GUI Flash Tool** from the toolbar to program the executable to the board.

![MUCXpresso_Flash](image/MCUXpresso_Flash.png)

### 3.4 Select low power mode and wake up configurations
- Open a serial terminal.
- Follow the prompts and enter one from A to E to enter different low power mode.

![select_low_power_mode](image/select_low_power_mode.png)

- Follow the prompts and enter one from 1 to 3 to select the wake up mode.

![select_wake_up_mode](image/select_wake_up_mode.png)

- Prompts the selected low power mode and wake up mode, the corresponding reference wake up time and low power current, and press SW3 on FRDM-MCXA153 to wake up the MCU. 
 
  **Please only press the wakeup button when prompt message appears, otherwise it will result in failure to wake up!**

![all_prompts](image/all_prompts.png) 

### 3.5 Measure low power current
- Use MCU-Link Pro and MCUXpresso IDE to measure low power current:
  - Connect MCU-Link Pro board to FRDM-MCXA153 board.
  
  |MCU-Link Pro|FRDM-MCXA153|
  |--|--|
  |J9-1|JP2-1|
  |J9-3|JP2-2|
  |J9-2|J3-14|

  - Follow below steps to measure current with MCUXpresso IDE.

![MCUXpresso_energy_measurement](image/MCUXpresso_energy_measurement.png)

- You can also use an ammeter to measure the current at JP2 of the FRDM-MCXA153 board.

### 3.6 Measure wake up time
- Get the wake up time by measuring the delay between the falling edges of J1-1 (P1_7) and J4-12 (P3_30) using an oscilloscope.

![measure_wake_up_time](image/measure_wake_up_time.png)


## 4. Results<a name="step4"></a>
The following wake up time and low power current are provided as a reference:

>P represents production sample, E represents engineering sample.

>The typical wake up time correspond to low power wake up time in the datasheet.

>Temperature, measuring instrument and wake up source etc. can affect wake up time.

>There is no special gate all peripherals clock, and DeepPowerDown mode retained all RAM, so the measured current will be a little different from the data in the datasheet.

|Power mode|Wake up mode|Wake up time(P)|Wake up time(E)|Power consumption(P & E)|
|--|--|--|--|--|
|Sleep|Typical|0.27us|0.27us|1.72mA|
|Sleep|Fast|0.14us|0.14us|3.27mA|
|Sleep|Slow|1.04us|1.04us|0.82mA|
|DeepSleep|Typical|7.52us|4.61us|22.1uA|
|DeepSleep|Fast|5.90us|2.65us|965.2uA|
|DeepSleep|Slow|14.59us|11.98us|22.0uA|
|PowerDown|Typical|17.26us|13.99us|6.2uA|
|PowerDown|Fast|7.79us|4.49us|202.8uA|
|PowerDown|Slow|39.74us|36.89us|6.2uA|
|DeepPowerDown|Typical|2.35ms|2.76ms|1.1uA|

## 5. FAQs<a name="step5"></a>
*No FAQs have been identified for this project.*

## 6. Support<a name="step6"></a>
*Please contact NXP for additional support.*

#### Project Metadata
<!----- Boards ----->
[![Board badge](https://img.shields.io/badge/Board-FRDM&ndash;MCXA153-blue)](https://github.com/search?q=org%3Anxp-appcodehub+FRDM-MCXA153+in%3Areadme&type=Repositories)

<!----- Categories ----->
[![Category badge](https://img.shields.io/badge/Category-LOW%20POWER-yellowgreen)](https://github.com/search?q=org%3Anxp-appcodehub+low_power+in%3Areadme&type=Repositories) [![Category badge](https://img.shields.io/badge/Category-POWER%20CONVERSION-yellowgreen)](https://github.com/search?q=org%3Anxp-appcodehub+power_conversion+in%3Areadme&type=Repositories)

<!----- Peripherals ----->
[![Peripheral badge](https://img.shields.io/badge/Peripheral-CLOCKS-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+clocks+in%3Areadme&type=Repositories) [![Peripheral badge](https://img.shields.io/badge/Peripheral-UART-yellow)](https://github.com/search?q=org%3Anxp-appcodehub+uart+in%3Areadme&type=Repositories)

<!----- Toolchains ----->
[![Toolchain badge](https://img.shields.io/badge/Toolchain-MCUXPRESSO%20IDE-orange)](https://github.com/search?q=org%3Anxp-appcodehub+mcux+in%3Areadme&type=Repositories)

Questions regarding the content/correctness of this example can be entered as Issues within this GitHub repository.

>**Warning**: For more general technical questions regarding NXP Microcontrollers and the difference in expected funcionality, enter your questions on the [NXP Community Forum](https://community.nxp.com/)

[![Follow us on Youtube](https://img.shields.io/badge/Youtube-Follow%20us%20on%20Youtube-red.svg)](https://www.youtube.com/@NXP_Semiconductors)
[![Follow us on LinkedIn](https://img.shields.io/badge/LinkedIn-Follow%20us%20on%20LinkedIn-blue.svg)](https://www.linkedin.com/company/nxp-semiconductors)
[![Follow us on Facebook](https://img.shields.io/badge/Facebook-Follow%20us%20on%20Facebook-blue.svg)](https://www.facebook.com/nxpsemi/)
[![Follow us on Twitter](https://img.shields.io/badge/Twitter-Follow%20us%20on%20Twitter-white.svg)](https://twitter.com/NXP)

## 7. Release Notes<a name="step7"></a>
| Version | Description / Update                           | Date                        |
|:-------:|------------------------------------------------|----------------------------:|
| 1.0     | Initial release on Application Code Hub        | January 30<sup>th</sup> 2024 |

