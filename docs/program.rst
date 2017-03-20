Programming Your Keypad
=======================

The Easy Way (Web Editor)
*************************

Arduino recently introduced their web-based editor, a more accessible version of the Arduino IDE that can run in your web browser. This is great for me because it's a lot easier than installing the program, libraries, copying and pasting code, selecting the port, selecting the board, etc. Don't worry about any of that, since we get to avoid all of it!

Requirements
------------

Browser: Chrome, Firefox, or Safari

Arduino account

Making an Arduino Account
-------------------------

Don't worry, they won't bombard you with emails (yet*) and you don't need to give them any personal information, they just require an account so you can manage your code. You can make your account from this page here:

`Sing Up <https://id.arduino.cc/auth/signup>`_

You will have to verify your account through your email (don't use a fake one) and then you're done!

Installing the browser plugin
-----------------------------

Go to the `Web Editor <https://create.arduino.cc/editor>`_ and you will be asked to install the plugin.

.. image:: https://puu.sh/teWji/8c567035ed.png


*On the welcome screen there's a box you have to uncheck if you don't want their emails:

.. image:: http://puu.sh/tg4Ko/6fcbdbad99.png

Drivers?
--------

If you're on a Windows machine running Windows 8.1 or lower you'll need drivers for your keypad to be recognized. The Arduino plugin should ask you to install them, but if it doesn't, you can follow my driver installation guide here:

`Driver Installation <http://docs.thnikk.moe/en/latest/driver.html>`_

Code
----

You're almost done, all that's left is loading the code. Fortunately, they have made this very easy. All you have to do is click on the share link for your corresponding keypad and add to sketchbook. It should then take you to the editor and tell you that Arduino Leonardo has been connected. You can now click on Upload (the right arrow) to upload the code to the keypad. If it works, you should see "Success: Done Uploading." If not, make sure the keypad is plugged in and that you installed the drivers.

.. image:: https://puu.sh/teWtO/df51db652d.png

`Basic <https://create.arduino.cc/editor/thnikk/d531845e-0521-4da9-b285-3bada89af0a6/preview>`_

`LED <https://create.arduino.cc/editor/thnikk/64bb5abf-7f2d-425f-a0df-9515604b22d5/preview>`_

`RGB <https://create.arduino.cc/editor/thnikk/a891c370-23ac-45bc-8499-0b68f5b8cc98/preview>`_

`4K <https://create.arduino.cc/editor/thnikk/743ed14a-3926-4c3c-8b9b-38056e09ea76/preview>`_

`4K RGB <https://create.arduino.cc/editor/thnikk/730ced35-bc57-4b74-8e8e-05a4c5d64942/preview>`_

The Hard Way (Desktop IDE)
**************************

If you don't want to use the Web IDE, you can download the tried-and-true desktop IDE instead. This is a little harder and requires a few extra steps, but you don't need to make an account.

Driver Installation
-------------------
If you're on a Windows machine running Windows 8.1 or lower you'll need drivers for your keypad to be recognized.

`Driver Installation <http://docs.thnikk.moe/en/latest/driver.html>`_

Installing the Arduino IDE
--------------------------
The latest version of the Arduino IDE can be downloaded and installed from the Ardiono website here:

`Arduino.cc <https://www.arduino.cc/en/Main/Software>`_

Configuring the IDE
-------------------

A few things need to be done before you start. Firstly, you will need the libraries used in the code, otherwise it can't compile because all of that code will be missing. This used to be a lot more difficult to install, but since version 1.6.5 they added the Library Manager, which allows you to install libraries directly from the IDE. To install libraries, launch the IDE open the Library Manager by going to Sketch > Include Library > Manage Libraries...

.. image:: http://thnikk.moe/guides/reprogram/img/libman.jpg

My code uses the Bounce2 and NeoPixel libraries, so just search "Bounce2" and "Adafruit NeoPixel" in the search bar and click Install on the result. After they are installed, you can close the Library Manager window.

Verifying and Uploading the Code
--------------------------------

Delete the code in the main window of the Arduino IDE. Now, paste in the code for your corresponding keypad from my GitHub here:

`GitHub <https://github.com/thnikk/newKeypad>`_

Now we need to tell the IDE what it's trying to compile for and communicate with. Go to Tools > Board and select Arduino Leonardo and go to Tools > Port and select the one that has (Arduino Leonardo) to the right of it.

.. image:: http://thnikk.moe/guides/reprogram/img/board.jpg

Now you can click on the right arrow to compile and upload the code. Don't worry about uploading broken code, if it fails to compile it won't upload it to the microcontroller. It will ask you to save and you can either click cancel or choose a directory to save to, it will compile either way. If you didn't get any errors, congratulations, you did it! If you did, make sure you've selected the board and the port and that there's nothing left behind from the blank sketch or that you copied the code correctly.


Making Changes to the Firmware
******************************

Understanding the basics
------------------------

There are some things you should familiarize yourself with before you get started. These are the most important functions/libararies.

`digitalRead() <https://www.arduino.cc/en/Reference/DigitalRead>`_ for reading button inputs.

`analogWrite() <https://www.arduino.cc/en/Reference/AnalogWrite>`_ for controlling single-color LEDs.

The `neopixel library <https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library>`_ for controlling RGB LEDs.


You should also know...
-----------------------

`for loops <https://www.arduino.cc/en/Reference/For>`_

`switch/case statements <https://www.arduino.cc/en/Reference/SwitchCase>`_

`millis timers <https://learn.adafruit.com/multi-tasking-the-arduino-part-1/using-millis-for-timing>`_


I wouldn't recommend trying to change random variables unless you can actually follow the logic and see exactly what they're changing. Some of them may tie in somewhere else and cause unexpected results.
