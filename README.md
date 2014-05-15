[cmft](https://github.com/dariomanesku/cmft) - cubemap filtering tool
=====================================================================

What is it?
-----------

Cross-platform open-source command-line cubemap filtering tool.<br \>
Utilizes both host CPU and OpenCL GPU at the same time for fast processing! (check [perfomance charts](https://github.com/dariomanesku/cmft#Performance)) <br \>

- Supported input/output formats: \*.dds, \*.ktx, \*.hdr, \*.tga.
- Supported input/output types: cubemap, cube cross, latlong, face list, horizontal strip.

![cmft-cover](https://github.com/dariomanesku/cmft/raw/master/res/cmft-cover.jpg)

Download
--------

Download binaries here: <br \>
 * [cmft - Windows 32bit](https://github.com/dariomanesku/cmft-bin/raw/master/cmft_win32.zip)<br />
 * [cmft - Windows 64bit](https://github.com/dariomanesku/cmft-bin/raw/master/cmft_win64.zip)<br />
 * [cmft - Linux 32bit](https://github.com/dariomanesku/cmft-bin/raw/master/cmft_lin32.zip)<br />
 * [cmft - Linux 64bit](https://github.com/dariomanesku/cmft-bin/raw/master/cmft_lin64.zip)<br />
 * cmft - OSX 64bit (to come...)<br \>
 * cmft - OSX 64bit<br \>

Building
--------

	git clone git://github.com/dariomanesku/cmft.git
	cd cmft
	make

After calling `make`, \_projects folder will be created with all suppored project files. Deleting \_projects folder at any time is safe.<br \>
- For Windows, Visual Studio solution can be found in \_projects/vs2012/<br \>
- For OS X, XCode solution can be found in \_projects/xcode4/<br \>
- For Linux, Makefile can be found in \_projects/gmake-linux-gcc/<br \>
- Also other compilation options are available, have a look inside \_projects directory.<br \>

- Projects can be also build from the root directory by running something like `make linux-gcc-release64`. Have a look at Makefile for details.<br \>
- All compiler generated files will be in \_build folder. Again, deleting \_build folder is safe at any time.<br \>
- File 'config.mk' is used for setting environment variables for different compilers.<br \>
- On Windows use Visual Studio, on Linux use gcc toolkit. For OS X, wait for a while. Ignore other options for now as they are not yet properly set up and may not work out-of-the-box as expected without some care.<br \>

Using
-----
Use runtime/cmft\_win.bat on Windows runtime/cmft\_lin.sh on Linux and runtime/cmft\_osx.sh on OSX as a starting point. First, compile the project, then edit runtime/cmft\_\* file as needed and then run it.

Project status
--------------

- Windows build works fine.
- Linux gcc build works but processing on CPU is noticeably slower comparing to Windows build (haven't yet figured out why). OpenCL runs fine.
- OSX binaries are not yet build. This will taken care of very soon.

### Known issues
- PVRTexTool is not properly opening mipmapped \*.ktx files from cmft. This appears to be the problem with the current version of PVRTexTool. Has to be further investigated.

Performance
-----------

cmft was compared with the popular CubeMapGen tool for processing performance.
Test machine: Intel i5-3570 @ 3.8ghz, Nvidia GTX 560 Ti 448.

### Filter settings:
- Gloss scale: 8
- Gloss bias: 1
- Mip count: 8
- Exclude base: false

### Test case #1:
- Src face size: 256
- Dst face size: 256
- Lighting model: phongbrdf

### Test case #2:
- Src face size: 256
- Dst face size: 256
- Lighting model: blinnbrdf

### Test case #3:
- Src face size: 512
- Dst face size: 256
- Lighting model: phongbrdf

### Test case #4:
- Src face size: 512
- Dst face size: 256
- Lighting model: blinnbrdf

|Test case| CubeMapGen   | cmft Cpu only | cmft Gpu only | cmft |
|:--------|:-------------|:--------------|:------------- |:---- |
|#1       |1:27.7        |0:08.6         |0:18.7         |0:06.0|
|#2       |4:39.5        |0:29.7         |0:19.6         |0:11.2|
|#3       |5:38.1        |0:33.4         |1:03.7         |0:21.6|
|#4       |18:34.1       |1:58.2         |1:07.7         |0:35.5|

![cmft-performance-chart](https://github.com/dariomanesku/cmft/raw/master/res/cmft-performance-chart.png)


Recommended tools
-----------------

[PVRTexTool](http://community.imgtec.com/developers/powervr/) - for opening \*.dds and \*.ktx files.<br />
[GIMP](http://www.gimp.org) - for opening \*.tga files.<br />
[Luminance HDR](http://qtpfsgui.sourceforge.net/) - for opening \*.hdr files.<br />

Environment maps
----------------

For testing purposes, you can use environment maps from [here](https://github.com/dariomanesku/environment-maps).

Similar projects
----------------

[CubeMapGen](http://developer.amd.com/tools-and-sdks/archive/legacy-cpu-gpu-tools/cubemapgen/) - A well known for cubemap filtering from AMD.<br \>
[Marmoset - Skyshop](http://www.marmoset.co/skyshop) - Commercial plugin for Unity3D Game engine.

Useful links
------------

1. [Sebastien Lagarde Blog - AMD Cubemapgen for physically based rendering](http://seblagarde.wordpress.com/2012/06/10/amd-cubemapgen-for-physically-based-rendering/)
2. ...more to come

Contributors
------------

* Cover photo design by Marko Radak. Check out his [personal website](http://markoradak.com/).

More to come
------------

- cmftViewer for viewing filtered cubemaps in action. cmftViewer will be as well cross-platform and open-source implemented using [bgfx rendering library](https://github.com/bkaradzic/bgfx/).
- Tutorial and details on theory and implementation.

Get involved
------------

In case you are using cmft for your game/project, please let me know. Tell me your use case, what is working well and what is not. I will be happy to help you using cmft and also to fix possible bugs and extend cmft to match your use case.

Other than that, everyone is welcome to contribute to cmft by submitting bug reports, feature requests, testing on different platforms, profiling and optimizing, etc.

When contributing to the cmft project you must agree to the BSD 2-clause licensing terms.

Keep in touch
-------------

Add me and drop me a line on twitter: [@dariomanesku](https://twitter.com/dariomanesku).


[License (BSD 2-clause)](https://github.com/dariomanesku/cmft/blob/master/LICENSE)
-------------------------------------------------------------------------------

    Copyright 2014 Dario Manesku. All rights reserved.

    https://github.com/dariomanesku/cmft

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

       1. Redistributions of source code must retain the above copyright notice,
          this list of conditions and the following disclaimer.

       2. Redistributions in binary form must reproduce the above copyright notice,
          this list of conditions and the following disclaimer in the documentation
          and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDER ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
    EVENT SHALL COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
    OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
    ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
