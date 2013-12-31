Riffer!

A framework designed for reading and writing large files in the RIFF format.

The assumption is that there are many top-level chunks in the RIFF format. We can think of these as being like frames in a video.
Each of these will have several sub-chunks, also RIFF format. Nesting beyond this is not supported out of the box.

Test code is included that demonstrates really basic use.

This is like the 4th time I have re-written this code from the ground up, and so I decided to make it open-source. A previous version of this code was used for an accepted CHI 2014 submission on live video editing.



