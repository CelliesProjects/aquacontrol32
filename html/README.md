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
'index.htm' now looks like:

```c
cellie@cellie-Mint-64 ~ $ cat index.htm
<h1>Simple webpage</h1>
```
To convert this to a C-style header file use:
```c
cellie@cellie-Mint-64 ~ $ xxd -i index.htm > index_htm.h
```
Which produces:
```c
cellie@cellie-Mint-64 ~ $ cat index_htm.h
unsigned char index_htm[] = {
  0x3c, 0x68, 0x31, 0x3e, 0x53, 0x69, 0x6d, 0x70, 0x6c, 0x65, 0x20, 0x77,
  0x65, 0x62, 0x70, 0x61, 0x67, 0x65, 0x3c, 0x2f, 0x68, 0x31, 0x3e, 0x0a
};
unsigned int index_htm_len = 24;
```
Now change the produced file so the `unsigned char` declaration are of `const` type.
To save some more bytes, you can remove the last line.

The above code should look like:
```c
const char index_htm[] = {
  0x3c, 0x68, 0x31, 0x3e, 0x53, 0x69, 0x6d, 0x70, 0x6c, 0x65, 0x20, 0x77,
  0x65, 0x62, 0x70, 0x61, 0x67, 0x65, 0x3c, 0x2f, 0x68, 0x31, 0x3e, 0x0a
};
```
Now the data is saved in flash memory instead of precious ram memory.

One more step to do...

#### Step 2: Add terminator zero byte to all pages:

Web pages are stored sequentially in flash memory, without any terminator or space between them, so before the code is compiled to memory a terminator NULL byte `0x00` has to be added to each webpage, otherwise the 'ESPAsyncWebServer' will serve them as a single blob.

If your pages look funny and are all served at once, good chance you forgot this terminator!

#### The above example without terminator byte:
```c
const char index_htm[] = {
  0x3c, 0x68, 0x31, 0x3e, 0x53, 0x69, 0x6d, 0x70, 0x6c, 0x65, 0x20, 0x77,
  0x65, 0x62, 0x70, 0x61, 0x67, 0x65, 0x3c, 0x2f, 0x68, 0x31, 0x3e, 0x0a
};
```
#### The above example with a terminator byte added:
```c
const char index_htm[] = {
  0x3c, 0x68, 0x31, 0x3e, 0x53, 0x69, 0x6d, 0x70, 0x6c, 0x65, 0x20, 0x77,
  0x65, 0x62, 0x70, 0x61, 0x67, 0x65, 0x3c, 0x2f, 0x68, 0x31, 0x3e, 0x0a,
  0x00 /* << Note this extra byte! */
};
```
The sketch expects the web interface files in the folder `webif`.
Save `index_htm.h` in the `webif` folder and...

#### Ready!

Now you can compile and upload your sketch.

