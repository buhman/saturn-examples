import sys
import struct

with open(sys.argv[1], 'rb') as f:
    buf = f.read()

stride = 320

def get_tile(buf, tx, ty):
    tile = []
    for y in range(8):
        row = []
        for x in range(8):
            yy = ty * 8 + y
            xx = tx * 8 + x
            ix = yy * stride + xx
            px = buf[ix]
            row.append(px)
        tile.append(tuple(row))
    return tuple(tile)

def tile_input(buf):
    tiles = []
    pattern = []

    for ty in range(240 // 8):
        for tx in range(320 // 8):
            tile = get_tile(buf, tx, ty)
            #if tile in tiles:
            #    pass
            #else:
            #    tiles.append(tile)
            #tile_ix = tiles.index(tile)
            tiles.append(tile)
            tile_ix = len(tiles) - 1
            pattern.append(tile_ix)

    return tiles, pattern


def emit_tile(tile):
    for row in tile:
        for ix in range(len(row) // 2):
            a = row[ix * 2 + 0]
            b = row[ix * 2 + 1]
            c = ((a & 0xf) << 4) | ((b & 0xf) << 0)
            yield c

def emit_tiles(tiles):
    for tile in tiles:
        yield from emit_tile(tile)

def emit_tile_data(tiles):
    b = bytes(emit_tiles(tiles))
    with open(sys.argv[2], 'wb') as f:
        f.write(b)

def emit_pattern_data(pattern):
    with open(sys.argv[3], 'wb') as f:
        for ix in pattern:
            assert ix < 1200, ix
            b = struct.pack('>H', ix)
            f.write(b)

assert len(sys.argv) == 4, len(sys.argv)
tiles, pattern = tile_input(buf)
emit_tile_data(tiles)
emit_pattern_data(pattern)
