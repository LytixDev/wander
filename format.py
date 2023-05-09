import os

for root, dirs, files in os.walk("src"):
    for file in files:
        if file.endswith((".c", ".h")):
            os.system("clang-format -i -style=file " + root + "/" + file)
