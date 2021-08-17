# Zeuz Password Manager (ZPM)
A simple password manager based on a Arduino Nano

![ZPM Image](/Photos/IMG_20210817_132327.jpg)

This password manager allows to store and display a set of username/passowrds for different accounts.
The device is protected by a PIN that must be input on boot.
Check this [short video](https://youtu.be/qvDMkEPs7oM) for a demo.

The security is questionable because:
1. Retrieving and analyzing the Arduino ROM will reveal the credentials.
2. Brute forcing the PIN is relatively easy.

To mitigate the risk a bit you can keep the same (or diferent) set of digits for yourself.
Alternatively there are other password managers, check [PasswordPump](https://www.hackster.io/dan-murphy/passwordpump-passwords-manager-7c6d84), it stores our encrypted passwords in external ROM modules and has a lot more features.

It was a funny project to build anyway.

All source files included, check the photos for guiding the build.
