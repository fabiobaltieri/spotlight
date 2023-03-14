#!/usr/bin/env python3

import os
import time

HOME = os.environ["HOME"]
MCUMGR = f"sudo {HOME}/go/bin/mcumgr -c spotlight"
IMAGE_PATH = "build/zephyr/zephyr.signed.bin"
OFFSET = -328

image_hash = ""
with open(IMAGE_PATH, mode="rb") as fp:
    content = fp.read()
    for b in content[OFFSET:OFFSET+32]:
        image_hash += "%02x" % b

def run(cmd):
    print(cmd)
    os.system(cmd)

run(f"{MCUMGR} image upload -e {IMAGE_PATH}")
run(f"{MCUMGR} image test {image_hash}")
run(f"{MCUMGR} reset")
time.sleep(15)
run(f"{MCUMGR} image confirm {image_hash}")
run(f"{MCUMGR} image erase")
