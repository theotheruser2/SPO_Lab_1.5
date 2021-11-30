#!/usr/bin/python3

import random


def generate_command_name():
    return random.choice(['get', 'set', 'new', 'del'])


def generate_path():
    segments = []

    for i in range(random.randint(0, 10)):
        segments.append(random.choice(['a', 'test', 'foo', 'bar', 'oaoaoa', 'kek', 'lol', 'b', '0', '1']))

    value = random.randint(-100, 100) if random.randint(0, 10) == 0 else None
    value = f' = "{value}"' if value is not None else ''

    return '.'.join(segments) + value


def generate_command():
    return generate_command_name() + ' ' + generate_path()


try:
    while True:
        print(generate_command())

except KeyboardInterrupt:
    pass
