# Riffer 

Riffer is a framework for storing RIFF (Resource Interchange File Format) data, built especially for:
* high performance on large files
* seamless, simultaneous read/write access
* flexible data types
* lack of file corruption

The motivation to create Riffer was inspired by the difficulty (in 2012) of using existing video container file formats (a) for live video editing, and (b) with non-standard data streams, such as depth data.

## Format Details

The assumption is that there are many top-level chunks (```rfr::Chunk```) in the RIFF format. We can think of these as being like frames in a video.
Each of these will have several sub-chunks, also RIFF format. Nesting beyond this is not supported out of the box.

Test code is included that demonstrates basic use. Adding to and reading from Chunk objects looks like so:
    
    rfr::CaptureSession cs("./capture.dat");
    rfr::Chunk chunk("colour frame");
    int width = 640; int height = 480;
    chunk.add_parameter("width", width);
    chunk.add_parameter("height", height);
    
    int got_width == *(chunk_by_index.get_parameter<int>("width"));
    
    cs.add(chunk);
    cs.close();

This is like the 4th time I have re-written this code from the ground up, and so I decided to make it open-source. Previous versions have all been written in C#, so some vestigial C# idioms may appear.

## Projects
This code was used in the following:

LACES, a touch-based video editor presented at CHI 2014. Video demo here: https://www.youtube.com/watch?v=9zG--3M2DMs

Background Activity, a depth and colour dataset of natural user behaviour in a living room environment. http://www.dgp.toronto.edu/~dustin/backgroundactivity/

Improv Remix, a gestural performance tool for theatre performers to do live video editing of stage environments. https://dustinfreeman.org/improv-remix/

### Downstream Dependencies
The following projects depend on Riffer.

https://github.com/dustinfreeman/kriffer, which stores Windows Kinect SDK data in riffer

https://github.com/dustinfreeman/nfig, which uses a ```rfr::Chunk``` object as a set of configurations for an application. nfig serializes in the JSON format. 


