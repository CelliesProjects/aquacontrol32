### This are the web interface files for aquacontrol32.

This folder contains the raw html files for the web interface.

Before compiling, these files have to be converted to C style header files.

You can convert these files to `.h` files with the linux commandline tool `xxd`.

These files are then used as `#include` files in `webserver.ino`.

Converted files should be saved in the folder `webif`.

Follow the instructions below to produce valid files for use with aquacontrol32.


#### Step 1: Convert to C header file:

Make a file:
```c
cellie@cellie-Mint-64 ~ $ echo '<h1>Simple webpage</h1>' > index.htm
```
`index.htm` now looks like:

```c
cellie@cellie-Mint-64 ~ $ cat index.htm
<h1>Simple webpage</h1>
```
To convert this to a C-style header file use:
```c
cellie@cellie-Mint-64 ~ $ xxd -i index.htm > index_htm.h
```
Which produces `index_htm.h` that looks like:
```c
cellie@cellie-Mint-64 ~ $ cat index_htm.h
unsigned char index_htm[] = {
  0x3c, 0x68, 0x31, 0x3e, 0x53, 0x69, 0x6d, 0x70, 0x6c, 0x65, 0x20, 0x77,
  0x65, 0x62, 0x70, 0x61, 0x67, 0x65, 0x3c, 0x2f, 0x68, 0x31, 0x3e, 0x0a
};
unsigned int index_htm_len = 24;
```
Now change this file so the `unsigned char` is changed to `const uint8_t`.

The above code should look like:
```c
const uint8_t index_htm[] = {
  0x3c, 0x68, 0x31, 0x3e, 0x53, 0x69, 0x6d, 0x70, 0x6c, 0x65, 0x20, 0x77,
  0x65, 0x62, 0x70, 0x61, 0x67, 0x65, 0x3c, 0x2f, 0x68, 0x31, 0x3e, 0x0a
};
unsigned int index_htm_len = 24;
```
Now the data is saved in flash memory instead of precious ram memory.

The sketch expects the web interface files in the folder `webif`.
Save `index_htm.h` in the `webif` folder and...

#### Ready!

Now you can compile and upload your sketch with the changed `index.htm`.

