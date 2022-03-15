#!/usr/bin/env python3

import requests

PREAMBLE = '''
#include "EmojiTable.h"

#ifdef FULL_EMOJI_TABLE

namespace unicode {

EmojiTable EmojiTable::create()
{
    EmojiTable inst;
'''

EPILOGUE = '''
    return inst;
}
}
#endif
'''


# goes through lines in a file and for those which pass the line_filter calls line_callback function
def process_file_lines(filename, line_filter, line_callback):
    with open(filename, mode="r") as f:
        for _, raw_line in enumerate(f):
            if line_filter(raw_line):
                line_callback(raw_line)


def parse_code_point(value):
    return int(value, 16)


def parse_code_point_range(range_desc):
    if ".." in range_desc:
        bounds = range_desc.split("..")
        if len(bounds) != 2:
            raise ValueError("invalid range descriptor")
        return parse_code_point(bounds[0]), parse_code_point(bounds[1])

    cp = parse_code_point(range_desc)
    return cp, cp


def parse_emoji_data_file(filename):
    ranges = []

    def want_line(line):
        return len(line) != 0 and not line.startswith("#")

    def line_callback(line):
        parts = line.strip().split(';')

        range_raw = parts[0].strip()
        if len(range_raw) == 0:
            return

        cp_range = (0, 0)

        try:
            cp_range = parse_code_point_range(range_raw)
        except ValueError as e:
            print("invalid input: {} | {}".format(e, line))
            return

        if not is_disabled(cp_range):
            ranges.append(cp_range)

    process_file_lines(filename, want_line, line_callback)

    return ranges


def is_disabled(range_desc):
    return range_desc[0] <= 0xff


def format_code_point(cp):
    return "0x{:4X}".format(cp)


def format_range(range_desc):
    beg, end = range_desc
    inner_range_str = format_code_point(beg)
    if beg != end:
        inner_range_str += ", {}".format(format_code_point(end))

    return "    inst.add({});\n".format(inner_range_str)


def generate(source_filename, output_filename, source_url):
    ranges = parse_emoji_data_file(source_filename)

    sorted_ranges = sorted(ranges, key=lambda rng: rng[0])

    lines = [format_range(range_rec) for range_rec in sorted_ranges]

    with open(output_filename, mode="w") as f:
        f.write("// generated from: {}".format(source_url))
        f.write(PREAMBLE)
        f.write("    inst.table_.reserve({});\n\n".format(len(lines)))
        f.writelines(lines)
        f.write(EPILOGUE)


def main():

    url = "https://unicode.org/Public/UNIDATA/emoji/emoji-data.txt"
    data_filename = "emoji-data.txt"
    output_filename = "EmojiTable-full.gen.cpp"

    resp = requests.get(url)

    with open(data_filename, "wb") as f:
        f.write(resp.content)

    generate(data_filename, output_filename, url)


if __name__ == '__main__':
    main()
