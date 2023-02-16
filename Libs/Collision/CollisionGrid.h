#pragma once
#include <vector>

// A 3D grid of cells, where each cell holds information about three of its sides.
// To be as memory-efficient as possible, cells are 4-bit structures where each bit represents:
// Bit 0: Floor exists
// Bit 1: West wall exists
// Bit 2: South wall exists
// Bit 3: Is step (floor is raised 0.5 units - surrounding cells with floor will make a half-wall up)
// Special case (bits !0 and 3): Unused
// Floors and walls are always double-sided. A cell's east wall is it's east neighbour's west wall.
// A single int therefore represents 8 cells, arranged in a 2x2x2 grid called a 'block'.
// A 64-bit cache line fits 16 ints, or 128 cells.
// Assuming most terrain will be relatively flat, the ints will be stored in chunks of 4x1x4 blocks,
// stored by x, then z, then y.
// For this reason, width and depth must be multiples of 8, while height must be a multiple of 2.
// It is also faster to iterate over areas of the grid in groups of 8x2x8.
class CollisionGrid
{
public:
    /** Constructs a collision grid with the given dimensions. Height must be a multiple of 2, width and depth must be multiples of 8. */
    CollisionGrid(int width, int height, int depth)
    {
        if (width % 8 != 0) return; // error
        if (height % 2 != 0) return; // error
        if (depth % 8 != 0) return; // error
        m_width = width;
        m_height = height;
        m_depth = depth;
        m_yOffFactor = GetChunkWidth() * GetChunkDepth();
        m_zOffFactor = GetChunkWidth();
        m_blocks.resize(m_width * m_height * m_depth / 8);
    }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    int GetDepth() const { return m_depth; }

    int GetChunkWidth() const { return m_width / 8; }
    int GetChunkHeight() const { return m_height / 2; }
    int GetChunkDepth() const { return m_depth / 8; }

    /** Sets the 4-bit collision data for the given cell. */
    void SetCell(int x, int y, int z, int data)
    {
        int blockOffset = GetBlockOffset(x, y, z);
        int cellOffset = GetCellOffset(x, y, z);
        int block = m_blocks[blockOffset] & ~(0xF << cellOffset);
        m_blocks[blockOffset] = (block | (data << cellOffset));
    }

    /** Sets the 4-bit collision data for the given cell. Uses precomputed offsets, useful when you want to operate on the same cell multiple times in a row. */
    void SetCell(int x, int y, int z, int data, int blockOffset, int cellOffset)
    {
        int block = m_blocks[blockOffset] & ~(0xF << cellOffset);
        m_blocks[blockOffset] = (block | (data << cellOffset));
    }

    /** Returns the 4-bit collision data for the given cell. */
    int GetCell(int x, int y, int z) const
    {
        int block = m_blocks[GetBlockOffset(x, y, z)];
        return (block >> GetCellOffset(x, y, z)) & 0xF;
    }

    /** Returns the 4-bit collision data for the given cell. Uses precomputed offsets, useful when you want to operate on the same cell multiple times in a row. */
    int GetCell(int x, int y, int z, int blockOffset, int cellOffset) const
    {
        int block = m_blocks[blockOffset];
        return (block >> cellOffset) * 0xF;
    }

    /** Returns the array index of the block containing the given cell in the internal data structure. */
    inline int GetBlockOffset(int x, int y, int z) const
    {
        int chunkIndex = (m_yOffFactor * (y >> 1)) + (m_zOffFactor * (z >> 3)) + (x >> 3);
        return (chunkIndex << 4) + ((z & 7) << 1) + ((x & 7) >> 1);
    }

    /** Returns the offset of the cell in bits within its containing block. */
    inline int GetCellOffset(int x, int y, int z) const
    {
        return ((y & 1) << 4) | ((z & 1) << 3) | ((x & 1) << 2);
    }

private:
    /** Array of blocks (2x2x2 cells), stored in groups of 4x1x4 for cache efficiency */
    std::vector<int> m_blocks;

	int m_width = 0;
	int m_height = 0;
	int m_depth = 0;
    int m_yOffFactor = 0;
    int m_zOffFactor = 0;
    const int BLOCK_SIZE = 4 * 4;

    // A chunk is 4x1x4 blocks, or 8x2x8 cells.
    // This chunk struct is not actually used, as it is faster to lookup blocks as an int array (unless we static_cast the Chunk array to int[]?)
    //struct Chunk
    //{
    //    int blocks[16];
    //};
    //Chunk chunks[];

    // Let's say we want to get the cell at 31,7,24 in a 64x32x64 grid (x,y,z respectively).
    // If blocks were stored in a basic 3D int array, the block offset would be:
    //  ((64/2)*(64/2)*(7/2)) + ((64/2)*(24/2)) + (31/2) (= 3072 + 384 + 15 = 3471th element)

    // Since blocks are stored in chunks:
    // The chunk offset is:
    //  ((64/8)*(64/8)*(7/2)) + ((64/8)*(24/8)) + (31/8) (= 448 + 24 + 3 = 475)
    // The block offset within the chunk is:
    //  (((24%8)*4)/2) + ((31%8)/2) = 7

    // We can flatten this out to get the block's array in the flat index.
    // The block offset would be:
    //  (width/8)*(depth/8)*(y/2) + (width/8)*(z/8) + (x/8) + ((z%8)*2) + ((x%8)/2)
 
    // The cell offset within the block would still be:
    //  ((7%2)*16) + ((24%2)*8) + ((31%2)*4) = 24 bits

    // Simplify:
    //  const int CHUNKS_WIDTH = width/8;
    //  const int CHUNKS_HEIGHT = height/8;
    //  const int Y_OFF = CHUNKS_WIDTH * CHUNKS_HEIGHT;
    //  const int Z_OFF = CHUNKS_WIDTH;
    //  block offset = (Y_OFF * (y>>1)) + (Z_OFF * (z>>3)) + (x>>3) + ((z&7)<<1) + ((x&7)>>1)
    //  cell offset = (((y&1)<<4) | ((z&1)<<3) | ((x&1)<<2))
};

