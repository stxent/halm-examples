#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# make_example.py
# Copyright (C) 2023 xent
# Project is distributed under the terms of the GNU General Public License v3.0

'''Generate example files from templates for specified platform.

This module generates C example files from Jinja2 templates.
'''

import argparse
import os
import jinja2

def parse_config(text):
    options = {}
    entries = text.split(',')

    for entry in entries:
        parts = entry.split('=')

        if len(parts) != 2:
            continue

        if parts[1] in ('true', 'false'):
            options[parts[0]] = parts[1] == 'true'
            continue

        try:
            options[parts[0]] = int(parts[1], 10)
            continue
        except ValueError:
            pass

        try:
            options[parts[0]] = int(parts[1], 16)
            continue
        except ValueError:
            pass

        options[parts[0]] = parts[1]

    return options

def make_example(platform, family, bundle, config, name):
    path = os.path.dirname(os.path.abspath(__file__))
    path_templates = os.path.abspath(f'{path}/../templates')
    options = parse_config(config)

    model = {
        'config': options,
        'group': {
            'platform': platform,
            'family': family,
            'name': bundle
        }
    }

    env = jinja2.Environment(loader=jinja2.FileSystemLoader(path_templates),
                             newline_sequence='\r\n',
                             keep_trailing_newline=True)
    return env.get_template(f'{name}.jinja2').render(model)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--alias', dest='alias', help='output example name', default='')
    parser.add_argument('--bundle', dest='bundle', help='bundle name', default='')
    parser.add_argument('--config', dest='config', help='code configuration options', default='')
    parser.add_argument('--family', dest='family', help='processor family name', default='')
    parser.add_argument('--output', dest='output', help='output directory', default='')
    parser.add_argument('--platform', dest='platform', help='platform name', default='')
    parser.add_argument(dest='templates', nargs='*')
    options = parser.parse_args()

    for name in options.templates:
        text = make_example(options.platform, options.family, options.bundle, options.config, name)

        if options.output:
            output_name = options.alias if options.alias else name
            example_path = os.path.abspath(f'{options.output}/{output_name}')
            os.makedirs(example_path, exist_ok=True)

            with open(f'{example_path}/main.c', 'wb') as stream:
                stream.write(text.encode())
        else:
            print(text)

if __name__ == '__main__':
    main()
