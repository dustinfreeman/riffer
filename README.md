Riffer is a framework for storing flexible data types into chunks, and reading and writing large files with high-performance in the RIFF format, including seamless read/write access.

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


This is like the 4th time I have re-written this code from the ground up, and so I decided to make it open-source. A previous version of this code was used for LACES, a touch-based video editor presented at CHI 2014. Video demo here: https://www.youtube.com/watch?v=9zG--3M2DMs
Previous versions have all been written in C#, so some vestigial C# idioms may appear that I'm trying to weed out.

Riffer is used by two other projects of mine:

https://github.com/dustinfreeman/kriffer
which stores Windows Kinect SDK data in riffer
AND
https://github.com/dustinfreeman/nfig
which uses a ```rfr::Chunk``` object as a set of configurations for an application. nfig serializes in the JSON format. 


