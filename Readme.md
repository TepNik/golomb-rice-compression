Golomb-Rice coding compressor
======================================

This is the implementation of Golomb-Rice coding. You can find information about this algorithm [here](https://monkeysaudio.com/theory.html).

# File Compression

## Compile program
        make

## Compress file
        ./golomb input_file

## Flags

##### 8, 16, 32 or 64 bit compression flags
        -i8, -i16, -i32 or -i64

##### Custom output file (by default it is input_file.golomb)
        -o output_file

##### Set variable k to new value n. For 8, 16, 32 and 64 bit compression default value for k is 4, 8, 16 and 32 respectively.
        -k=n

##### Make int signed or unsigned
        -sign=true
        -sign=false
