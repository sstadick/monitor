import os
import subprocess
import sys
import time


def walk_files(dir_path, exts):
    for root, dirs, files in os.walk(dir_path):
        for file in files:
            file_path = os.path.join(root, file)
            ext = os.path.splitext(file_path)[1]
            if ext[1:] in exts:
                yield file_path

def main():
    if len(sys.argv) < 3:
        print("Missing exts and dir to watch", file=sys.stderr)
    exts = set(sys.argv[1].split(','))
    files = {}

    dir_to_watch = sys.argv[2]
    print(f"Monitoring for exts({exts}) in {dir_to_watch}", file=sys.stderr)
    os.chdir(dir_to_watch)
    while True:
        for file in walk_files(dir_to_watch, exts):
            with open(file, 'r') as fh:
                contents = fh.read()
                if files.get(file, False):
                    if contents == files[file]:
                        continue
                    files[file] = contents
                    # run the comand
                    print("change detected running command", file=sys.stderr)
                    subprocess.run(["make"])
                else:
                    files[file] = contents
        time.sleep(.1)



if __name__ == '__main__':
    main()
