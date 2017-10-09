# SoundOverSerial
** DISCLAIMER **
I AM NOT RESPONSIBLE FOR DAMAGE TO YOUR HEARING, HARDWARE OR ANYTHING ELSE IF YOU USE THIS APPLICATION/PROGRAM! ITS A CONCEPT, SO EXCPECT THE UNEXPECTED!

SoundOverSerial, play sound from your computer to your stm32f0 over usart/serial. It sends over everything you would normally hear over your speakers, without installing drivers. With practically no delay :D.

  - Magic

## Download

[Download zip here!](https://github.com/EldinZenderink/SoundOverSerial/releases/tag/v1.0)

## How to use!

  - Make sure your audio device is set to 48000hz/16-bit as shown here:
  - ![alt text](https://i.imgur.com/AUbhFpP.png "Logo Title Text 1")
  - Make sure both dll and executable are together
  - Connect your stm32f0 discovery using a serial to usb converter
  - Connect your speaker/speaker out/3.5mm out jack
        -For **PWM**: Use pin PC7
        -For **DAC**: Use pin PA4
  - Launch the executable and follow the instructions there!


Warning (seriously scary loud noise can occur)!:
  - Make sure your volume is not to high when either (best to disconnect/reconnect after these changes are applied):
    - Switching on the stm32f0
    - Connecting via the executable
    - Switching from PWM to DAC and vice versa application side
    - Switching from PWM to DAC and vice versa on the microcontroller
    

### Tech

* [NAudio](https://github.com/naudio/NAudio) - Awesome audio library for C#/.NET
