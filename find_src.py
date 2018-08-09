
import os, sys

r = sys.argv[1]
for root, dirs, files in os.walk(r):
    for f in files:
        if len(f) < len(".cpp"):
            continue
        if f[-4:] != ".cpp":
            continue
        print(os.path.join(root, f))
