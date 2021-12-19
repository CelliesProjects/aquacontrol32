### This are the web interface files for aquacontrol32.

This folder contains the html files for the web interface.

Before compiling these files have to gzipped and after that converted to C style header files.

You can convert these files to `.h` files with the linux commandline tool `xxd`.

These files are then used as `#include` files in `webserver.ino`.

Converted files should be saved in the folder `webif`.

Follow the instructions below to produce valid files for use with aquacontrol32.

If you are on another OS than Linux, you probably will have to use some other tools than the ones in this description.

### How to prepare web interface files for use with aquacontrol32 (in Linux)

####  Step 1: Make a html file

Make a file:

```c
cellie@cellie-Mint-64 ~ $ echo '<h1>Simple webpage</h1>' > index.htm
```

#### Step 2: Gzip the file

Use `gzip` in a terminal (or the context menu in your filebrowser) to gzip the file.

`cellie@cellie-Mint-64 ~ $ gzip --keep index.htm`

This will produce the file `index.htm.gz`.

#### Step 3: Convert the gzipped file to a C-style header file

To convert this to a C-style header file use:

```c
cellie@cellie-Mint-64 ~ $ xxd -i index.htm.gz > index_htm_gz.h
```

Which produces `index_htm_gz.h` that looks like:

```c
cellie@cellie-Mint-64 ~ $ cat index_htm.h
unsigned char index_htm_gz[] = {
  0x1f, 0x8b, 0x08, 0x08, 0xae, 0x00, 0xbf, 0x61, 0x00, 0x03, 0x69, 0x6e,
  0x64, 0x65, 0x78, 0x2e, 0x68, 0x74, 0x6d, 0x00, 0xb3, 0xc9, 0x30, 0xb4,
  0x0b, 0xce, 0xcc, 0x2d, 0xc8, 0x49, 0x55, 0x28, 0x4f, 0x4d, 0x2a, 0x48,
  0x4c, 0x4f, 0xb5, 0xd1, 0x07, 0x0a, 0x71, 0x01, 0x00, 0x68, 0x24, 0x4d,
  0xb1, 0x18, 0x00, 0x00, 0x00
};
unsigned int index_htm_gz_len = 53;
```

##### In the example above the file actually increased in size, but with a more realistic case, it usually will shrink in size.

#### Step 4: Change the code to `const` to save to flash memory

Change the first line to `const unsigned char`.<br>
Change the last line to `const unsigned int`.

The above code should look like:

```c
const unsigned char index_htm_gz[] = {
  0x1f, 0x8b, 0x08, 0x08, 0xae, 0x00, 0xbf, 0x61, 0x00, 0x03, 0x69, 0x6e,
  0x64, 0x65, 0x78, 0x2e, 0x68, 0x74, 0x6d, 0x00, 0xb3, 0xc9, 0x30, 0xb4,
  0x0b, 0xce, 0xcc, 0x2d, 0xc8, 0x49, 0x55, 0x28, 0x4f, 0x4d, 0x2a, 0x48,
  0x4c, 0x4f, 0xb5, 0xd1, 0x07, 0x0a, 0x71, 0x01, 0x00, 0x68, 0x24, 0x4d,
  0xb1, 0x18, 0x00, 0x00, 0x00
};
const unsigned int index_htm_gz_len = 53;
```
This will compile the data to flash memory instead of precious ram memory.

The sketch expects the web interface files in the folder `webif`.
Save `index_htm_gz.h` in the `webif` folder and...

#### Step 5: Profit!

Now you can compile and upload your sketch with the changed `index.htm`.

