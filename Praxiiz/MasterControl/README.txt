[ MasterControl.dll <initial release> ]

[ What is MasterControl.dll? ]

MasterControl.dll is a packet handler for UO clients.  It provides an 
interface for sending and receiving messages between the game client and
the injection client.  It's open source and written in C++.  No joke, this
is the very first C/C++ I've ever written, so I'm sure I made mistakes.

[ Features? ]

* Self injecting, just pass a process handle to the Inject function
* Intercepts all incoming and outgoing packets
* Ability to send raw packets to server or client
* Has incoming and outgoing packet filters which can be changed on the fly
  These filters drop the packets from being received by client/server while
  also giving the ability to silently modify them and resend
* Ability to call client macros (speech, skills, spells, etc.)
* Ability to use client's built in pathfind function to specified x,y,z
* Ability to execute gump vtable functions (simulate button press, close, etc.)
* Tested and confirmed working on current EA servers, and of course RunUO
* Some basic client patches that we've all come to rely on

[ Patches you say? ]

* MultiUO patch which allows multiple clients to run at the same time
* Encryption removal
* Login server patch, allows you to set login server you want to use

Notes on the patches:  I've tested all supported clients to make sure
that all the byte signatures (also ones in Pointers.cpp) used in MasterControl
are 1.) present and 2.) not present more than once.  There are a couple
exceptions (patches only) but they're confirmed to be working.  One other thing,
the MultiUO patch for a handful of versions around the 4.0.0p mark only partially
works.  Those clients have a built in limit of 2 simultaneous clients.  I can 
fix it, but I'm not going to right now.

[ Supported clients ]

Everything from 3.0.6m to 7.0.24.2 (and probably some future versions)
That's 200+ clients.
All features are supported in these clients.

[ How do I write my own application using MasterControl.dll? ]

Included with MasterControl.dll is a C# application which shows some
crude examples of implementing the functionality available.  It's not
pretty, but it works.

[ FAQ ]

* Will you add support for other clients?  How about any other features?
- Maybe, maybe not.  Who knows.

* Will you provide a decently coded example application?
- Don't hold your breath.

* Do you do requests?
- No.

* Do you want anyone to send questions about this to your personal email?
- No.

[ Cheers! ]

- xenoglyph