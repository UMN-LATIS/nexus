<h2>Documentation</h2>

<ul>
<li>Man pages: <a href="man/nxsbuilder.html">nxsbuilder</a>, 
  <a href="man/nxsedit.html">nxsedit</a>, <a href="man/nxsview.html">nxsview</a></li>
<li><a href="roadmap.php">known bugs and roadmap</a></li>
<li><a href="classes.html">class documentation</a></li>
<li><a href="format.html">nexus format</a></li>
<li><a href="technical.html">technical report (very preliminary!)</a></li>
</ul>

<h3>Basic usage</h3>
<h4>Create the model</h4>
<p>Use <em>nxsbuilder</em> to create a multiresolution nexus model (.nxs) out of your mesh (.ply):</p>
<pre class="prettyPrint">
$ nxsbuilder bunny.ply
</pre>
<p>The result will be <em>bunny.nxs</em>. For large files this may take quite some time. See the <a href="man/nxsbuilder.html">man page</a> for all the options, supported input files etc.</p>

<p>You can get some statistics on the created model: bounding sphere, list of patches created along with their error using nxsedit:</p>

<pre class="prettyPrint">
$ nxsedit bunny.nxs -i
</pre>

<h4>Simplify and compress</h4>

<p>Quite often the meshes comes out of a marching cube algorithm, and are quite redundant, 
you can prune the lowest level of the resolution using the -l option.</p>
<p>Another possibility is to compress the meshes, saving aroung 80% of the size, but slowing
down initial loading of the model. This is most useful for streaming applications. When compressing you need to specify a vertex quantization grid step (usually 1/5th of the marching cube step is a good value.</p>

<pre class="prettyPrint">
$ nxsedit bunny.nxs -l -z -v 0.01 -o bunnyZ.nxs
</pre>
<p>Detailed information about the objects can be found in the <a href="man/nxsedit.html">man page</a>.

<img style="float:right" src="img/nxsview.png"/>
<h4>Inspect your model.</h4>
<p>Nxsview is a simple program for inspecting a .nxs file and test rendering speed in various situations:</p>
<pre class="prettyPrint">
$ nxsview bunny.nxs 
</pre>
<p>You can tune various parameters through the interface, but be sure to read the available options in the <a href="man/nxsedit.html">man page</a></p>
<p>Nxsview can also be used across http: put your model on a webserver and access it remotely:</p>
<pre class="prettyPrint">
$ nxsview http://vcg.isti.cnr.it/nexus/bunny.nxs
</pre>

<hr/>
<h3 style="clear:both"><a name="use"></a>Use nexus inside your application</h3>
<p>classes (Nexus for the model, the Controller singleton, the instance.
point to class documentation</p>
<p>Usage example using QGLWidget... chiacchierare con lo spat per rendere la cosa semplice da usare con le varie versioni di OpenGL e gli shaders...<p>
<hr/>
