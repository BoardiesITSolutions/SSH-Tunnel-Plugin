First of all, thank you for your interest in Boardies MySQL Manager for Android and the PHP and SSH Tunnelling APIs. This is the source code for the SSH Tunnelling Plugin

This readme file will outline how to use the project but more information can be found on the GitHub page. If you need
any help with anything, such as making changes to the code, installing the APIs on your own server, then please ask the question
via GitHub or via the support portal support ticketing system at https://support.boardiesitsolutions.com. 

We would love to hear your feedback and see what changes and enhancements the open source community wish to make, but please be gentle with me,
this is my first C++ application coming from a C#/Java/PHP background :). 

***Using the Source Code***
The project consists of a Visual Studio Solution file and project file, so can be opened straight into Visual Studio - 2017 minimum. 
You can use a different IDE if you prefer when making changes, but when uploading the changes to GitHub, please ensure that your own IDE
project files are not uploaded to GitHub and that the Visual Studio project files remain and are still intact. 

Note that you might need to change the project properties to point to the correct location of the dependency libraries to 
match the location of where they are stored on your own development environment. 

Note that there is also a makefile located in the route of the project source. This makefile is not used by Visual Studio but is instead the file 
that is used by the Linux compiler - more on that in a sec.

***Prerequsits***
There are several dependencies that are required in order to compile the source code. 
Whether its on Linux or Windows, the following dependency libraries are required:
- OpenSSL
- Boost Library including:
	- Boost Filesystem
	- Boost System
- Libssh2
- RapidJson
- C++11 Compiler
How to compile and link to the libraries are outside the scope of this readme - follow the libraries documentation on how to compile

***Compiling for Windows***
As mentioned Visual Studio can be used - minimum version of Visual Studio 2017 is required. You might need to change the project properties to match the location
of where your dependency libraries are stored. You can use your own IDE but ensure your IDE files are not added to the project on GitHub. 

The project has only been compiled on x64 bit Windows 10, there should be no reason that I can think of as to why it wouldn't work on 32 bit but as Boardies IT Solutions 
is just me, I don't have another x86 copy of Windows to be able to try it on. If you have/need this to run on a 32 bit operating system, and find that it does not work, then
please feel free to make any necessary changes - but please ensure it doesn't break the x64 and Linux platform compatibility. 

***Compiling for Linux***
You need the following tools installed on Linux in order to build the source code
- GCC
- G++
- Make
- GDB (Not necessarily required, but might be useful if you need to debug)
Ensure that any dependencies are installed for the above tools. 
The make file is used in order to compile the source code. Go into the root directory of where the .cpp and .h files are and run make clean to remove any already compiled object files
followed by make. This will ensure everything is compiled from scratch, while just make changes to the code, you can just run make to compile what's changed instead of make clean followed 
by make. 

***Notes About Making Changes***
In order to push changes to GitHub, you must follow the points below otherwise the change might be rejected
- Ensure that any changes do not break compatibility between the Windows and Linux platforms, if a change is made, that is only supported on 1 particular platform,
unless it provides a big improvement or requirement for the platform it works on it will likely get rejected. All efforts must be made to ensure that compatibility
between Linux and Windows is maintained.
- Test the code for memory leaks, don't worry, if you miss one too much, its easily done, but please try to ensure memory leaks do not occur due to your code changes
- If you're using your own IDE - if you're developing for Linux you have to, but please ensure that any specific IDE files or directories that are created are not pushed
to GitHub. 
