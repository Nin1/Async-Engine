# Async-Engine
Experimental C++ game engine designed to scale up in multi-core systems

Uses lock-free programming to create a multi-stage pipeline through which multiple frames can be executed simultaneously, using a scalable job system to parallelise heavy tasks.
Per-frame data is passed through the pipeline one stage at a time in a single "FrameData" struct, so each frame does not impact the ones before/after it.
Loaded assets and shaders are shared between frames.

Lock-free job system heavily inspired by https://blog.molecular-matters.com/2015/08/24/job-system-2-0-lock-free-work-stealing-part-1-basics/
Engine architecture inspired by https://www.gdcvault.com/play/1022186/Parallelizing-the-Naughty-Dog-Engine
