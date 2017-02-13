BUILD:

You will need QT development packages (4.8 at least).

Glew library is also required. In Ubuntu you can install it with:

sudo apt-get install libglew-dev

Libcurl is optional for remote (through http) inspections of models in nxsview (see option USE_CURL inside nxsview.pro).
Install it in ubuntu with

sudo apt-get install libcurl4-openssl-dev

You should be able just to load the .pro files (nxsbuild, nxsedit and nxsview) in qtcreator and compile straight away.



BASIC USAGE

* Create the model

Use nxsbuild to create a multiresolution nexus model (.nxs) out of your mesh (.ply):

$ nxsbuild bunny.ply

The result will be bunny.nxs. For large files this may take quite some time. See the nxsbuilder man for all the options, supported input files etc.
You can get some statistics on the created model: bounding sphere, list of patches created along with their error using nxsedit:

$ nxsedit bunny.nxs -i


* Simplify

Quite often the meshes comes out of a marching cube algorithm, and are quite redundant, you can prune the lowest level of the resolution using the -l option.

$ nxsedit bunny.nxs -l  -o bunny1.nxs

Detailed information about the objects can be found in the man page. 


* Compress

$ nxsedit bunny.nxs -z -o bunny.nxz

Will compress the file (about 1/8th) with standard configuration parameters. Check man page for options.


* Inspect your model.

Nxsview is a simple program for inspecting a .nxs file and test rendering speed in various situations:

$ nxsview bunny.nxs 

You can tune various parameters through the interface, but be sure to read the available options in the man page

Nxsview can also be used across http (provided you compiled it using the USE_CURL option): put your model on a webserver and access it remotely:

$ nxsview http://vcg.isti.cnr.it/nexus/bunny.nxs


* Use nexus inside your application

There are a couple of sample applications in the source code: nxsview itself and sample. Nexus is compatible with standard OPENGL and with OpenGL ES.



