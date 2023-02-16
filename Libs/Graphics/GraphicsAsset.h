#pragma once

enum class LoadState
{
	/** This asset has not been asked to load yet */
	UNLOADED,
	/** This asset is currently loading */
	LOADING,
	/** This asset has been loaded, but not yet uploaded to vram */
	LOADED,
	/** This asset has been loaded and uploaded to vram. Its data may not exist CPU-side. */
	UPLOADED,
	/** This asset attempted to load but failed */
	FAILED
};

/**
 * Base class for anything that is loaded from a file and uploaded to the GPU.
 * Assets follow a process for asynchronous loading:
 * 1. Calling code calls Load(filename) once.
 * 2. A job is created to read that file from disk
 * 3. The render thread can poll GetLoadState() until it returns LOADED
 * 4. The render thread calls Upload() to upload the asset to the GPU and delete it from CPU-side memory
 * 5. This asset then contains any CPU-side data needed thereafter
 * 
 * TODO: Make this a template class instead? Would be good if all files loaded in a packet format.
 */

