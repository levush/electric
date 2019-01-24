README Documentation
---
To generate the html documentation goto the root directory
of this project and issue
doxygen doc/Doxyfile
This takes a while and you need to have a recent graphviz 
---
The html documentation is really huge so it will not be in this repo
and it will not be updated too often.
See:
https://github.com/levush/electric-html-doc.git
And here github also has a limit of 100M on a single file so uploading
a >500MB tar ball of generated docs does not work there.
So the tar ball with the docs will need to stay somewhere else :-(
---
If you downloaded the html.tbz tar ball you can decompress it and surf the code:
The compressed tar ball contains the created doxygen documentation 
using dot/graphviz.
To surf the code you dont need graphviz or doxygen.
Just decompress bzcat html.tbz|tar -xvf - and untar it into the location
of this readme file.

cp /location_of_electric-html-doc/html.tbz .
bzcat html.tbz|tar -xvf -
if you want: rm html.tbz

Use the surfthecode.html page as start page
 as browsers want to generate a file list which 
takes long as there are many files in folder html.

