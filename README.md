# Zeuz_Password_Manager
A simple password manager based on a Arduino Nano

This password manager allows to store a display a set of passowrds.
The device is protected by a PIN that must be input on boot.

The security is questionable because:
1- Retrieving and analyzing the Arduino ROM will reveal the credentials.
2- Brute forcing the PIN is easy.

To mitigate the risk a bit you can keep the same (or diferent) set of digits for yourself.
Alternatively there are other password managers that store our encrypted passwords in external ROM modules.

It was a funny project to build anyway.

All source files included, check the photos for guiding the build.
