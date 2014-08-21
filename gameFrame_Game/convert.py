#!/usr/bin/python
import fnmatch, os, re, sys, stat, struct
from PIL import Image

def get_index(x, y):
    if y == 0:
        return 15 - x
    if y % 2 != 0:
        return y * 16 + x

    return (y * 16 + 15) - x

def convert(i):
    if i.size != (16, 16):
        raise Exception('Image must be 16x16.')

    buf = ['\0'] * (16*16*3)

    for y in range(0, 16):
        for x in range(0, 16):
            color = i.getpixel((x, y))
            idx = get_index(x, y) * 3

            # The LED color order is GRB.
            r = color[0]
            g = color[1]
            b = color[2]
            buf[idx+0] = chr(g)
            buf[idx+1] = chr(r)
            buf[idx+2] = chr(b)

    return ''.join(buf)

def go():
    input_path = sys.argv[1]
    output_path = sys.argv[2]
    os.stat(input_path).st_mode

    f = open(output_path, 'w')

    f.write(struct.pack('<l', 0x11221212))

    # File header size:
    f.write(struct.pack('<l', 0))

    def write_image(path):
        # Frame header:
        frame_len_ms = 1000/30
        header = struct.pack('<l', frame_len_ms)

        # Header size:
        f.write(struct.pack('<l', len(header)))
        f.write(header)

        image = Image.open(path)
        data = convert(image)
        f.write(data)

    if stat.S_ISDIR(os.stat(input_path).st_mode):
        print input_path
        files = [fn for fn in os.listdir(input_path) if fnmatch.fnmatch(fn, '*.bmp') or fnmatch.fnmatch(fn, '*.png') or fnmatch.fnmatch(fn, '*.jpg')]
        def sort_key(item):
            # Files may be of the form '0001.bmp' or '1.bmp'.  Most applications that
            # output image sequences use '0001.bmp' and the files automatically sort
            # correctly, but GameFrame uses '1.bmp' which means we have to do a bit of
            # extra work to get them to sort.
            item = os.path.basename(item)
            m = re.match(r'^([0-9]+)\.bmp', item)
            if m:
                return int(m.group(1))
            else:
                return item
        files.sort(key=sort_key)
        print files
        for fn in files:
            p = '%s/%s' % (input_path, fn)
            print p
            write_image(p)
    else:
        write_image(input_path)

go()
