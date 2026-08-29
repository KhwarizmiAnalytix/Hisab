#!/usr/bin/env python3
"""Embed kernels.metal into metal_kernels_source.h.in (CMake configure_file equivalent)."""

import sys
from pathlib import Path


_PLACEHOLDER = "@METAL_KERNEL_SOURCE@"


def main(argv: list[str]) -> int:
    if len(argv) != 4:
        sys.stderr.write(
            "usage: embed_metal_kernel.py TEMPLATE.metal.h.in KERNELS.metal OUTPUT.h\n"
        )
        return 2
    template_path = Path(argv[1])
    source_path = Path(argv[2])
    output_path = Path(argv[3])
    template = template_path.read_text(encoding="utf-8")
    source = source_path.read_text(encoding="utf-8")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(
        template.replace(_PLACEHOLDER, source),
        encoding="utf-8",
        newline="\n",
    )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
