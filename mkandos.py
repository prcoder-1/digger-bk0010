#!/usr/bin/env python3
"""Образ дискеты ANDOS с файлами игры.

    mkandos.py ЧИСТЫЙ.IMG ВЫХОД.IMG [+]файл.BIN ...

ANDOS — FAT12 (800 Кбайт: 80 дорожек x 2 стороны x 10 секторов x 512). Файл БК
кладётся без заголовка .BIN (адрес, длина): адрес загрузки ANDOS хранит в поле
времени записи каталога (смещение 22), дата 1980-01-01 (0x0021). Имя — имя
файла без .BIN. Образ побайтно совпадает с тем, что даёт `bkdecmd a` (BKDE).

ANDOS файл только загружает. «+» перед именем — автостарт, как у игр на дисках
ANDOS: файл грузится с 0760, а 8 слов 0760..0776 равны адресу старта и затирают
стек загрузчика — выход из загрузки уходит в программу.
"""
import os
import struct
import sys


def main():
    src, dst, files = sys.argv[1], sys.argv[2], sys.argv[3:]
    img = bytearray(open(src, 'rb').read())
    bps, spc, res, nfat, nroot, total = struct.unpack('<HBHBHH', img[11:21])
    spf = struct.unpack('<H', img[22:24])[0]
    fat_off = res * bps
    root_off = (res + nfat * spf) * bps
    data_off = root_off + nroot * 32
    nclusters = (total * bps - data_off) // (spc * bps) + 2
    csize = spc * bps
    fat = img[fat_off:fat_off + spf * bps]

    def fat_get(n):
        v = struct.unpack('<H', fat[n * 3 // 2:n * 3 // 2 + 2])[0]
        return v >> 4 if n & 1 else v & 0xFFF

    def fat_set(n, v):
        i = n * 3 // 2
        w = struct.unpack('<H', fat[i:i + 2])[0]
        w = (w & 0x000F) | (v << 4) if n & 1 else (w & 0xF000) | v
        fat[i:i + 2] = struct.pack('<H', w)

    def free_entry():
        for i in range(nroot):
            if img[root_off + i * 32] in (0, 0xE5):
                return root_off + i * 32
        sys.exit('mkandos: каталог полон')

    used = 0
    for path in files:
        auto = path.startswith('+')
        path = path.lstrip('+')
        raw = open(path, 'rb').read()
        addr, length = struct.unpack('<HH', raw[:4])
        body = raw[4:4 + length]
        if auto:
            if addr != 0o1000:
                sys.exit('mkandos: автостарт только для файлов с адресом 01000')
            body = struct.pack('<8H', *[addr] * 8) + body
            addr = 0o760
        name = os.path.basename(path).upper()
        if name.endswith('.BIN'):
            name = name[:-4]
        base, _, ext = name.partition('.')
        if len(base) > 8 or len(ext) > 3:
            sys.exit('mkandos: имя %s не укладывается в 8.3' % name)
        need = max(1, (len(body) + csize - 1) // csize)
        chain = [n for n in range(2, nclusters) if fat_get(n) == 0][:need]
        if len(chain) < need:
            sys.exit('mkandos: на диске нет места для %s' % name)
        for a, b in zip(chain, chain[1:] + [0xFFF]):
            fat_set(a, b)
        for k, n in enumerate(chain):
            part = body[k * csize:(k + 1) * csize]
            off = data_off + (n - 2) * csize
            img[off:off + csize] = part + bytes(csize - len(part))
        e = free_entry()
        img[e:e + 32] = (base.ljust(8).encode() + ext.ljust(3).encode() + b'\0' * 11 +
                         struct.pack('<HHHI', addr, 0x0021, chain[0], len(body)))
        used += need
    for k in range(nfat):
        img[fat_off + k * spf * bps:fat_off + (k + 1) * spf * bps] = fat
    open(dst, 'wb').write(img)
    print('%s: %d файлов, %d кластеров по %d байт' % (dst, len(files), used, csize))


main()
