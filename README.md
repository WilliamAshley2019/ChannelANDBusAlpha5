OK this is a work in progress but the concept works so far.  (however still buggy currently multiple audio stream processing is screwing up sonot working as intented on the multimemory streamaspect.. sonot functional yet but met the milestone)  I think it may relate to Bus's  memory pool as it may be configured to only handle a buffer of a single channels audio data quanity so I may need to dynamically allocate bus's share access pool to allow multistream channel inputs.

You must run your daw as admin as it uses windows.h shared global memory pool to route audio.

I am going to see about doing a local memory pool version to see if it can be used at user level maybe today as people may not want to run their daw as administrator.

Anywho. So how this plugin works.

Currently 

Add ChannelAlpha5 to any mixer track (tested in fl studio works in stereo mode not working about mono compatibility currently)

Add BusAlpha5 as a generator / synth 

The audio from Channel Alpha5 will be routed to Bus Alpha 5

You can pool multipe instances of channel into bus. 

Intent to add some bus features into bus suchas bus summing and other useful bus stuff.

How this differs from LoopbackAlpha   It uses multiple channels so there are 32 channel paths to route audio along. The idea is to have multiple instances of bus (up to 32 different channel routings)

loopback was a common path so it wasl ike one instance of loopback per session. The idea of ChannelAndBus is to allow multiple channels to route audio through. Plus adding some channel strip functions to channel in future versions.
The intent is also to add some special fxs modules links but havn't really considered that fully yet.

None the less I have hit the benchmark the administrator mode thing was driving me nuts as I couldn't figure out why it wasn't working until I thought that it may be privelege even though I run as administrator youstill need to launch the application
DAW as admin even if you yourself are admin as for whatever reason fl studio for instance doesn't run at a high enough elevation to access global shared memory pools aparently unless you give fl studio administrator access.

Anyway on to try to fix the user mode / local memory and see if it will even work, I sort of remember from loopback maybe this didn't work but I can't remember. 

Built with JUCE  8.0.12 using Microsoft Visual Studio 2026 should also build maybe in v17.
Quickly builds with plugin basics + dsp - I didn't bother to strip down modules. 



Builds
