## Description
A C program to list similar images based upon the Bhattacharya distance in the HSV colorspace.

It uses opencv and bdb and has a persistent server

## Installation

At least in ubuntu you can do

    $ sudo apt-get install libopencv-dev libdb-dev

## Usage

When you start it up, a server is run [based off another project of mine](https://github.com/kristopolous/proxy).

You can use a `-P` option to specify a port.

Go into the test directory and run

    $ ./convert.sh

This will create 26,000 images.

Now go back and run the server

    $ ./colors

You'll see an output like this:

         VV Number of concurrent connections

    8765,64,27436

    ^^ Port

            ^^^^^ PID

This means we can connect to port 8765. 

Go back into the test directory and run this

    ./feed.sh 8765

If you watch the colors server output you'll get stuff like this:

    
    ...
    (635) [_DEFAULT_] Adding test/image-10633.jpg
    (636) [_DEFAULT_] Adding test/image-10634.jpg
    (637) [_DEFAULT_] Adding test/image-10635.jpg
    (638) [_DEFAULT_] Adding test/image-10636.jpg
    ...


Now it's time to test it.  telnet back to the port and run a command like this:


    $ telnet localhost 8765
    Trying 127.0.0.1...
    Connected to localhost.
    Escape character is '^]'.
    *?test/image-11278.jpg*
    ["test/image-10094.jpg","test/image-10095.jpg","test/image-10272.jpg","test/image-10269.jpg","test/image-10446.jpg","test/image-10093.jpg","test/image-10271.jpg","test/image-10092.jpg","test/image-10459.jpg","test/image-10283.jpg","test/image-11100.jpg","test/image-10635.jpg","test/image-10430.jpg","test/image-10447.jpg","test/image-10924.jpg","test/image-10607.jpg","test/image-10622.jpg","test/image-10799.jpg","test/image-10282.jpg","test/image-10250.jpg","test/image-10602.jpg","test/image-10426.jpg","test/image-10605.jpg","test/image-10603.jpg","test/image-10458.jpg","test/image-10776.jpg","test/image-10634.jpg","test/image-10424.jpg","test/image-10425.jpg","test/image-10781.jpg","test/image-10781.jpg","test/image-10953.jpg","test/image-11099.jpg","test/image-10777.jpg","test/image-10610.jpg","test/image-10778.jpg","test/image-10923.jpg","test/image-10957.jpg","test/image-10957.jpg","test/image-10922.jpg","test/image-11130.jpg","test/image-10955.jpg","test/image-10955.jpg","test/image-10440.jpg","test/image-10600.jpg","test/image-10604.jpg","test/image-10604.jpg","test/image-11131.jpg","test/image-11131.jpg","test/image-10954.jpg","test/image-10810.jpg","test/image-10573.jpg","test/image-10397.jpg","test/image-10811.jpg","test/image-10779.jpg","test/image-10779.jpg","test/image-10956.jpg","test/image-10956.jpg","test/image-10780.jpg","test/image-10780.jpg","test/image-11133.jpg","test/image-11132.jpg","test/image-11132.jpg","test/image-10747.jpg","test/image-10746.jpg","test/image-10572.jpg","test/image-10396.jpg","test/image-10748.jpg","test/image-10221.jpg","test/image-10221.jpg","test/image-10220.jpg","test/image-10220.jpg","test/image-11204.jpg","test/image-10147.jpg","test/image-10146.jpg","test/image-10142.jpg","test/image-11012.jpg","test/image-10145.jpg","test/image-10322.jpg","test/image-10319.jpg","test/image-10148.jpg","test/image-10143.jpg","test/image-11188.jpg","test/image-10166.jpg","test/image-11203.jpg","test/image-10144.jpg","test/image-10165.jpg","test/image-10321.jpg","test/image-10149.jpg","test/image-10320.jpg","test/image-10164.jpg","test/image-10341.jpg","test/image-10515.jpg","test/image-10340.jpg","test/image-10691.jpg","test/image-10690.jpg"]
    ^\
    Connection closed by foreign host.
    $

And there you go.

## Contexts

You can add different contexts (like different dbs) by doing

    +(context):(file)

We can go back to the feed to show us this. Try this:

    $ ./feed.sh 8765 somestring

And in the colors output you'll see

    ..
    (db) [somestring] Finding context
    (db) [somestring] Context number 1
    (4073) [somestring] Adding test/image-12242.jpg
    ..

That means that you can search for it like so:

    ?somestring:test/image-12242.jpg


## Speed

Adding an image really depends on the size but is generally < 10ms.
Searching the database also really depends on the size, but with 1,000,000 entries it's about 5ms on a vintage 2008 machine.
