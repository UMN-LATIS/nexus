<h2>Known Bugs <img class="status" src="img/bug.png"/></h2>
<ul>
<li>Sometimes Rendering fps drops drammatically if drawing only first patch, why?</li>
<li>Nxsedit, recomputating error, there is a horrible hack at the end of the function.</li>
<li>updatePriorities() is  bottleneck for very large models. Need to profile it.</li>
<li>someone under windows reports a crash on exit.</li>
</ul>


<h2>Roadmap: plans for the future</h2>

<h3>Nxsbuilder</h3>

<h4>Finish support for point clouds <img class="status" src="img/favorite.png"/></h4>
<p>Needs only debugging and a decent clustering algorithm</p>

<h4>Make streaming more efficient</h4>
<p>Building the Kd-tree requires multiple pass:</p>
<ul>
<li> first pass: compute the bounding box</li>
<li> read 1 every 2^k triangles</li>
<li> read 1 every 2^k-1 triangles, skipping those already readed</li>
<li> ...</li>
<li> read 1 every 2 triangles </li>
</ul>
<p>We could od it in 2 passes instead: 1) compute bounding box and create a spatial occupancy
grid based on power of 2 spatial hashing. We can use the size of the triangle to determine the lowest
level of the grid. And use the grid to guide the subdivision of the nodes (instead of the triangles met so far).
</p>
<p>While we are at it we could add skip(n) to the triangle stream...</p>


<h4>More efficient quadric simplification</h4>
<p>Current quadric simplification algorith is not that fast, and moreover require converting back and forth from VCG mesh format. A native simplification algorithm could do faster and without conversions.</p>

<h4>Other fast simplification algorithms: vertex clustering</h4>
<p>For low quality but much faster processing of data.</p>

<h4>Profiling</h4>
<p>At the moment we have simplification as the most time-consuming process, but we could shave some time profiling and optimizing out some bottlenecks.
Speed currently (including simplification) is around 2 minutes per million triangles.</p>


<h3>Nxsproj</h3>
<p>Create projective texture multiresolution atlases: i.e. do it :)</p>

<h3>Nxsedit</h3>

<h4>Transformations</h4>
<p>Scaling, rotating, and translating the model...</p>

<h3>Nxsview</h3>

<h4>Point cloud using Gael algorithm <img class="status" src="img/important.png"/></h4>
<p>should be really easy... question is do we want to use a unifor (per patch) radius, 
or a per vertex one... should be an option probably... support for compression?...</p>

<h4>Streaming with local disk cache</h4>
<p>At the moment http streaming keeps data in RAM. we might want a permanent storage in disk...</p>
<p>There are some problem though, we need to know which patches has been downloaded and which not....
it is not worth to reorder the patches.... we also need some lock to syncornize reading cache with downloading cache,</p>

<h4>Make also the index multiresolution</h4>
<p>For very large models this might be worthwhile, or especially if we plan to load multiple very large models (a museum exibition with many statues for example....). Needs to be investigated. The difficult part would be the dag loading....</p>
    
<h4>Streamline reading from http or disk and decompression adding another cache stage <img class="status" src="img/warning.png"/></h4>
<p>At the moment downloading or reading from disk stalls when decompressing data.
adding another cache level would solve the problem. Need to split loadPatch into load and decompress.</p>

<h4>allows multiple cache transfers</h4>
<p>This would be useful for streaming from multiple sites, or discs!) 
semaphore to keep track of number of active transfers....</p>

<h4>Profile and further optimize mesh decompression <img class="status" src="img/warning.png"/></h4>
<p>Mesh decompression needs to be faster. find out what are the bottlenecks</p>

<h4>Collision detection</h4>
<p>We already have a bounding sphere hierarchy, the effort should be minimal...</p>

<h4>Occlusion culling</h4>
<p>Partially implemented using hardware occlusion queries. We plan to implement the Graphics Gem
(can't remeber name of the chapter). version, using temoral coherence and hierarchy of spheres.</p>

<h4>Miscellanous stuff</h4>
<p>Prioritize does not need to run every frame... and could be run on a different thread...
prioritizer should also extend the traversal by a % of the size of the visible, not a fixed number.
</p>
<p>Add callback for patch rendering...</p>

