#!/usr/bin/env python3

from hashlib import sha256

import os, sys

fw_files = [ 'DL168UV_V1.07_20250606.CDD', 'DL168UV_V1.07_20250606.CDI', 'DL168UV_V1.07_20250606.spi' ]
new_files = [ 'AnyTi3r-107.CDD', 'AnyTi3r-107.CDI', 'AnyTi3r-107.spi' ]
hashes = [
	'd4a1210dad490ff9ffbba1a9289d0b1a08c81b1e5814a64d1b78f587fcc5b520',
	'4b67b01addcdd7e667c075384b5f23cd1b80ff83c711f4afe79c549836afa3aa',
	'347f7e278eac27e2477b0f5f1bd899d17a774386fc31d29533d3b5f87a93bccd',
]
patches = [
	(0x0A3C8, 0x58, 0x00),
	(0x0A3C9, 0xB1, 0x25),
	(0x1166A, 0xFC, 0x2A),
	(0x1166B, 0xF7, 0xF0),
	(0x1166C, 0xC8, 0x81),
	(0x1166D, 0xFC, 0xFB),
]

firmware = os.listdir('.')

for i in [ 'DL168UV_V1.07_20250606.CDD', 'DL168UV_V1.07_20250606.CDI', 'DL168UV_V1.07_20250606.spi']:
	if i not in firmware:
		printf("Firmware file set is missing or incomplete!")
		sys.exit(1)

cdd = open(fw_files[0], 'rb').read()
cdi = open(fw_files[1], 'rb').read()
spi = open(fw_files[2], 'rb').read()

hasher = sha256()
hasher.update(cdd)
if hasher.hexdigest() != hashes[0]:
	print('Unexpected content in .CDD file!')
	sys.exit(1)

hasher = sha256()
hasher.update(cdi)
if hasher.hexdigest() != hashes[1]:
	print('Unexpected content in .CDI file!')
	sys.exit(1)

hasher = sha256()
hasher.update(spi)
if hasher.hexdigest() != hashes[2]:
	print('Unexpected content in .spi file!')
	sys.exit(1)

cdd = [byte for byte in cdd]

for patch in patches:
	if cdd[patch[0]] != patch[1]:
		printf("Unexpected byte at offset 0x%05X!" % patch[0])
	cdd[patch[0]] = patch[2]

cdd = bytes(cdd)
open(new_files[0], 'wb').write(cdd)
open(new_files[1], 'wb').write(cdi)
open(new_files[2], 'wb').write(spi)
