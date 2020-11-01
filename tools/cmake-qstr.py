#!/usr/bin/env python3

import argparse
import re
import sys
import io
from os import path as p, environ, system, chdir, makedirs, remove
from os.path import pardir, abspath, join, dirname, realpath, relpath, basename
from glob import glob
from contextlib import redirect_stdout

ARGS = None
BASEDIR = p.dirname(p.realpath(__file__))
HOMEDIR = p.normpath(p.join(BASEDIR, pardir))


class ArgProcessor(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        if values:
            values, = re.search(r'"(.*)"', values).groups()
            if option_string == "--sources":
                setattr(namespace, "sources", values.replace(";", " "))
            elif option_string == "--flags":
                setattr(namespace, "flags", values.replace(";", " "))
            elif option_string == "--defs":
                setattr(namespace, "defs", " ".join(["-D" + i for i in values.split(";") if i]))
            elif option_string == "--includes":
                setattr(namespace, "includes", " ".join(["-I" + i for i in values.split(";") if i]))


def exec(cmd):
    if ARGS.verbose:
        print(cmd)
    return system(cmd)


def save(content, filename):
    with io.open(filename, "wt", encoding="utf-8") as f:
        f.write(content)


def fprint(callable, filename):
    with io.open(filename, "wt", encoding="utf-8") as f:
        with redirect_stdout(f):
            callable()


def main():
    parser = argparse.ArgumentParser(
        description="cmake QSTRs generator wrapper")
    processor = parser.add_argument_group(
        "processing options, multiple values separated by semicolon enclosed by quotes")
    processor.add_argument("--compiler", required=True,
                           help="Compiler used for macro processing")
    processor.add_argument("--sources", required=True, action=ArgProcessor,
                           help="Collection of sources from which QSTRs would be extracted")
    processor.add_argument("--flags", action=ArgProcessor, default="",
                           help="Compiler flags passed to build micropython sources")
    processor.add_argument("--defs", action=ArgProcessor, default="",
                           help="Compiler definitions passed to build micropython sources")
    processor.add_argument("--includes", action=ArgProcessor, default="",
                           help="Collection of include directories passed to build micropython sources")
    processor.add_argument("--verbose", dest='verbose',
                           help="Print processing information")
    processor.add_argument("--qstrdefs", dest='qstrdefs',
                           help="Collection of files with extra/port defined QSTRs"
                           )

    generator = parser.add_argument_group("generator options")
    generator.add_argument("--outdir", default=BASEDIR,
                           help="Output directory where files will be generated")
    args = parser.parse_args()

    args.verbose = True
    global ARGS
    ARGS = args

    if ARGS.verbose:
        print(f" homedir: {HOMEDIR}")
        print(f"compiler: {ARGS.compiler}")
        print(f"   flags: {ARGS.flags}")
        print(f"    defs: {ARGS.defs}")
        print(f"  outdir: {ARGS.outdir}")
        print(f"includes: {ARGS.includes}")
        print(f" sources: {ARGS.sources}")
        print(f"qstrdefs: {ARGS.qstrdefs}")
        print()

    chdir(HOMEDIR)

    # step1
    makedirs(ARGS.outdir, exist_ok=True)
    exec(f"python py/makeversionhdr.py {ARGS.outdir}/mpversion.h")

    # step 2
    sys.path.append("py")
    from makemoduledefs import find_module_registrations, generate_module_table_header
    modules = set.union(*map(find_module_registrations, ARGS.sources.split()))
    fprint(lambda: generate_module_table_header(
        modules), f'{ARGS.outdir}/moduledefs.h')

    # step 3
    ret = exec(
        f"{ARGS.compiler} -E -DNO_QSTR {ARGS.defs} {ARGS.flags} {ARGS.includes} {ARGS.sources} > {ARGS.outdir}/qstr.i.last")

    if ret != 0:
        print("Compiling QSTR returns error.")
        for filename in glob("f{ARGS.outdir}/*.h"):
            remove(filename)
        sys.exit(ret)

    # step 4
    exec(f"python py/makeqstrdefs.py split qstr {ARGS.outdir}/qstr.i.last {ARGS.outdir}/qstr {ARGS.outdir}/qstrdefs.collected.h")
    exec(f"python py/makeqstrdefs.py cat qstr {ARGS.outdir}/qstr.i.last {ARGS.outdir}/qstr {ARGS.outdir}/qstrdefs.collected.h")

    # step 4
    exec(f'cat py/qstrdefs.h {ARGS.qstrdefs} {ARGS.outdir}/qstrdefs.collected.h | '
           f'sed \'s/^Q(.*)/"&"/\' | '
           f'{ARGS.compiler} -E {ARGS.defs} {ARGS.flags} {ARGS.includes} - | '
           f'sed \'s/^"\(Q(.*)\)"/\\1/\' > {ARGS.outdir}/qstrdefs.preprocessed.h')

    # step 5
    exec(f"python py/makeqstrdata.py {ARGS.outdir}/qstrdefs.preprocessed.h > {ARGS.outdir}/qstrdefs.generated.h")


if __name__ == '__main__':
    main()
