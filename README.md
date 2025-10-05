# WAVWSS

A simple C program that allows playing WAV files under DOS, using Windows Sound System-compatible sound card. Thanks to the simplicity of the WAV 
format, it's possible to use it to play CD-quality WAV files (44.1kHz, 16-bit stereo) even on 8088-based machines overclocked to ~6-7MHz.

## Features

* WAV file playback using a WSS-compatible sound card
* Support for both mono and stereo files
* Support for 8-bit and 16-bit PCM format
* Supported sample rates:
  - 8kHz
  - 11.025kHz
  - 16kHz
  - 22.05kHz
  - 32kHz
  - 44.1kHz
  - 48kHz
* Simple text-mode GUI with:
  - Pause/resume functionality
  - Previous/next track switching
  - Volume control
* Runs even on 8086/8088-based machines

## Motivation

Recently, completely by accident, I discovered the [Xi8088](https://github.com/skiselev/xi_8088) project - a complete, PC/XT-compatible processor board made 
entirely from through-hole components, primarily using 74xx series TTL chips. I decided to build it as a way to finally put my large stockpile of TTL chips 
(collected over years of tinkering with electronics) to good use, and to get hands-on experience with a "truly ancient" PC. I've never even seen a PC/XT in real 
life, let alone used one - I was born more than a decade too late :wink:

Of course, being a low-level tinkerer, I couldn't just build it, play _Prince of Persia_ and _Alley Cat_ for a while, then toss it into a drawer. I **had to**
set myself a challenge and write some at least vaguely useful software for the platform. Since I listen to a lot of music, and music is generally one of my 
greatest passions, I thought of just reviving my old project - [CDWSS](https://github.com/Lefucjusz/CDWSS) - which I wrote back when I was still a university student, 
just getting into retrocomputing with a 386 motherboard I saved from a trash bin near my apartment. 

After doing some research, I unfortunately found out that connecting an ATA CD drive to PC/XT wasn't possible due to the bus width mismatch - ATA drives require 
a 16-bit data bus, while the 8088 offers only 8 bits... Still, I decided not to give up and instead write a new program - based on what I learned from CDWSS - to
play WAV files from the hard disk. My crude calculations suggested it could work, even on such a resource-constrained machine.

I was a bit surprised that I couldn't find any existing software that could do this - which made me doubt whether my reasoning was correct - but decided to give it 
a shot anyway. After all, when there's no clear evidence that something is impossible, the only way to find out is to try - and that's a principle I live by.

To see if the idea even made sense, I tried it out on the same 386 PC I had used for CDWSS, partly to avoid dealing with the limitations of the Xi8088 right away, 
and because I already knew a similar concept had worked on that machine. Once I saw it was working as expected, I tried running it on the Xi8088. Following a few long 
nights of battling with what turned out to be a conflict between my VGA card and the PC/XT's quirky DMA implementation 
([see here for details](https://www.vogons.org/viewtopic.php?p=1367152#p1367152)), it finally worked! I was able to listen to CD-quality WAV files on a PC/XT - without 
any stuttering - which honestly surprised me.

## Usage

### Compiling

The program was written in **Borland C++ 2.0**. To compile it, clone the repo, open the `.PRJ` file inside the Borland C++ 2.0 IDE, and run `Compile` -> `Build All`.

Keep in mind that compiling this software _on_ the Xi8088 will take quite a long time - even up to a few minutes - depending on your CPU clock speed.

#### Note on an instruction set

By default, the project is configured to emit 8086/8088 instructions, making it compatible with the entire x86 family. If you plan to run it on some "newer" CPU, 
you can change this setting in `Options` -> `Compiler` -> `Code generation...` -> `More...` -> `Instruction Set` to target that CPU family. In theory, this allows 
the compiler to generate more optimized code, using the additional instructions available on that CPU.

However, keep in mind that doing so will break compatibility with older CPUs by introducing unsupported instructions. In practice, there's really no need to change 
it - any CPU above the 8086/8088 has more than enough power to run this program, even with only the basic instruction set.

### Resource configuration

> **Warning!** Be careful about the I/O value you set - the program does not perform any sanity checks and will blindly write to the provided
address as if it were a WSS base. In the worst-case scenario, this could even damage your hardware!

The default sound card resource configuration used by the program is:
* I/O: **0x530**
* IRQ: **5**
* DMA: **1**

To use a different configuration, define the `ULTRA16` environment variable in your `autoexec.bat` file:

```
SET ULTRA16=<I/O>,<IRQ>,<DMA>
```
* The I/O address should be provided as a hexadecimal value.
* IRQ and DMA values should be provided as decimal values.

#### Example
If your WSS card is at I/O `0x604`, uses IRQ line `7`, and DMA channel `3`, your `autoexec.bat` entry should look like:
```
SET ULTRA16=604,7,3
```
On startup, the program looks for this variable and attempts to parse it. If it's missing or malformed, the program will print an appropriate message and fall
back to default values:
```
ULTRA16 variable not found, using defaults (base 0x530, IRQ 5, DMA 1)
```
or
```
Malformed ULTRA16 variable, using defaults (base 0x530, IRQ 5, DMA 1)
```

### Running the program

The easiest way to run the program is to add the directory containing `WAVWSS.EXE` to your `PATH`, then run the `WAVWSS` command in the 
directory containing the WAV files you want to play. 

You can also run the program with a path to the files as an argument:
```
WAVWSS <directory_with_files_to_play>
```

After startup, if the provided directory contains valid WAV files, the program will show a list of them (using [natural sorting](https://github.com/sourcefrog/natsort))
and begin playing the first file. Once all files are played, the program will exit to DOS.

If the directory doesn't contain any valid WAV files, the program will terminate with an appropriate message.

### Controls

The program is controlled via keyboard:
* `ESC` - stop playback and exit to DOS
* `I` - previous track
* `O` - next track
* `P` - pause/resume
* `Y` - volume down
* `U` - volume up
