### This are the web interface files for aquacontrol32.

These files are added as `#include` in `webserver.ino`.

You can convert these files to `.h` files with the linux commandline tool `xxd`.

#### Step 1: Convert to C header file:

Make a file:

    cellie@cellie-Mint-64 ~ $ echo '<h1>Simple webpage</h1>' > index.htm

'index.htm' now looks like:

    cellie@cellie-Mint-64 ~ $ cat index.htm
    <h1>Simple webpage</h1>

To convert this to a C-style header file use:

    cellie@cellie-Mint-64 ~ $ xxd -i index.htm > index_htm.h

Which produces:

    cellie@cellie-Mint-64 ~ $ cat index_htm.h
    unsigned char index_htm[] = {
      0x3c, 0x68, 0x31, 0x3e, 0x53, 0x69, 0x6d, 0x70, 0x6c, 0x65, 0x20, 0x77,
      0x65, 0x62, 0x70, 0x61, 0x67, 0x65, 0x3c, 0x2f, 0x68, 0x31, 0x3e, 0x0a
    };
    unsigned int index_htm_len = 24;

Now change the produced file so the `unsigned char` declaration are of `const` type.

The above code should look like:

    const char index_htm[] = {
      0x3c, 0x68, 0x31, 0x3e, 0x53, 0x69, 0x6d, 0x70, 0x6c, 0x65, 0x20, 0x77,
      0x65, 0x62, 0x70, 0x61, 0x67, 0x65, 0x3c, 0x2f, 0x68, 0x31, 0x3e, 0x0a
    };
    const unsigned int index_htm_len = 24;

Now the data is saved in flash memory instead of precious ram memory.
To save some more bytes, you can remove the last line. ( `const unsigned...` )

One more step to do...

#### Step2: Add terminator zero byte to all pages:

Web pages are stored sequentially in flash memory, without any terminator or space between them, so before the code is compiled to memory a terminator NULL byte `0x00` has to be added to each webpage, otherwise the 'ESPAsyncWebServer' will serve them as a single blob.

If your pages look funny and are all served at once, good chance you forgot this terminator!

#### The above example without terminator byte:

    const char index_htm[] = {
      0x3c, 0x68, 0x31, 0x3e, 0x53, 0x69, 0x6d, 0x70, 0x6c, 0x65, 0x20, 0x77,
      0x65, 0x62, 0x70, 0x61, 0x67, 0x65, 0x3c, 0x2f, 0x68, 0x31, 0x3e, 0x0a
    };
    const unsigned int index_htm_len = 24;

#### The above example with a terminator byte added and last line removed:

    const char index_htm[] = {
      0x3c, 0x68, 0x31, 0x3e, 0x53, 0x69, 0x6d, 0x70, 0x6c, 0x65, 0x20, 0x77,
      0x65, 0x62, 0x70, 0x61, 0x67, 0x65, 0x3c, 0x2f, 0x68, 0x31, 0x3e, 0x0a,
      0x00
    };

#### Ready!

Now you can compile and upload your sketch.

