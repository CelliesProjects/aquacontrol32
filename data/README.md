### This are the web interface files for aquacontrol32.

You can convert these files to `.h` files with the linux commandline tool `xxd`.

These files are added as `#include` in `webserver.ino`.

#### Example:

    cellie@cellie-Mint-64 ~ $ echo '<h1>Simple webpage</h1>' > index.htm
    cellie@cellie-Mint-64 ~ $ cat index.htm
    <h1>Simple webpage</h1>
    cellie@cellie-Mint-64 ~ $ xxd -i index.htm > index_htm.h
    cellie@cellie-Mint-64 ~ $ cat index_htm.h
    unsigned char index_htm[] = {
      0x3c, 0x68, 0x31, 0x3e, 0x53, 0x69, 0x6d, 0x70, 0x6c, 0x65, 0x20, 0x77,
      0x65, 0x62, 0x70, 0x61, 0x67, 0x65, 0x3c, 0x2f, 0x68, 0x31, 0x3e, 0x0a
    };
    unsigned int index_htm_len = 24;

Now change the produced file so the declarations are of `const` type.

The above code should look like:

    const unsigned char index_htm[] = {
      0x3c, 0x68, 0x31, 0x3e, 0x53, 0x69, 0x6d, 0x70, 0x6c, 0x65, 0x20, 0x77,
      0x65, 0x62, 0x70, 0x61, 0x67, 0x65, 0x3c, 0x2f, 0x68, 0x31, 0x3e, 0x0a
    };
    const unsigned int index_htm_len = 24;
    
Now the data is saved in flash memory instead of precious ram memory.


