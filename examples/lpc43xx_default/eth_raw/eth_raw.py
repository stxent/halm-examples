#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# eth_raw.py
# Copyright (C) 2021 xent
# Project is distributed under the terms of the MIT License

'''Test script for the low-level ethernet driver.

This module sends a pseudorandom data to the test board,
then requests the data and checks the received response.
'''

import argparse
import errno
import random
import socket
import struct
import sys
import time
import psutil

ETHER_TYPE = 0x9000
OPCODE_BUF = 0x0001
OPCODE_REQ = 0x0002

def convert_mac(mac):
    parts = mac.split(':')
    return bytes([int(part, 16) for part in parts])

def serialize_mac(mac):
    return ':'.join(['{:02x}'.format(part) for part in mac])

def get_mac_address(interface):
    nics = psutil.net_if_addrs()

    try:
        for entry in nics[interface]:
            if entry.family == psutil.AF_LINK:
                return convert_mac(entry.address)
        return None
    except KeyError:
        return None

def decode_data_packet(packet):
    dst, src, eth_type = struct.unpack('!6s6sH', packet[0:14])

    if eth_type == ETHER_TYPE:
        op_code = struct.unpack('!H', packet[14:16])[0]

        if op_code == OPCODE_BUF:
            offset, length = struct.unpack('!II', packet[16:24])
            buffer = packet[24:24 + length]
            return src, dst, offset, buffer
    return None

def make_buffer(size):
    if size % 4 != 0:
        raise Exception()

    buffer = bytearray()
    for _ in range(0, size // 4):
        buffer += struct.pack('<I', random.randint(0, (1 << 32) - 1))
    return buffer

def make_data_packet(src, dst, buffer, offset):
    packet = struct.pack('!6s6sHHII', dst, src, ETHER_TYPE, OPCODE_BUF, offset, len(buffer))
    packet += buffer
    return packet

def make_data_request(src, dst):
    return struct.pack('!6s6sHHII', dst, src, ETHER_TYPE, OPCODE_REQ, 0, 0)

def receive_buffer(sock, src, dst, size):
    received_buffer = bytearray([0] * size)
    received_size = 0
    time_end = time.time() + 1.0

    while time.time() < time_end:
        try:
            packet, _ = sock.recvfrom(1536)
            fields = decode_data_packet(packet)
        except socket.timeout:
            fields = None

        if fields is not None:
            packet_src, packet_dst, offset, chunk = fields

            if packet_src == dst and packet_dst == src:
                if offset + len(chunk) <= len(received_buffer):
                    received_buffer[offset:offset + len(chunk)] = chunk
                    received_size += len(chunk)

            if received_size == size:
                break

    return received_buffer, received_size

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', dest='destination', help='MAC address of the target device',
        type=str, default='')
    parser.add_argument('-i', dest='interface', help='network interface',
        type=str, default='eth0')
    parser.add_argument('-s', dest='size', help='size of the memory region',
        type=int, default=16384)
    options = parser.parse_args()

    dst = convert_mac(options.destination)
    src = get_mac_address(options.interface)
    if src is None:
        print('Interface not found: {:s}'.format(options.interface))
        sys.exit(errno.ENODEV)

    buffer = make_buffer(options.size)
    chunk_size = 1024

    sock = socket.socket(socket.PF_PACKET, socket.SOCK_RAW, socket.htons(ETHER_TYPE))
    sock.settimeout(0.1)
    sock.bind((options.interface, 0))

    print('Sending {:d} bytes from {:s} to {:s}'.format(options.size,
        serialize_mac(src), serialize_mac(dst)))

    for i in range(0, options.size // chunk_size):
        chunk = buffer[i * chunk_size:(i + 1) * chunk_size]
        packet = make_data_packet(src, dst, chunk, i * chunk_size)
        sock.send(packet)

    packet = make_data_request(src, dst)
    sock.send(packet)

    received_buffer, received_size = receive_buffer(sock, src, dst, options.size)

    print('Received {:d} bytes'.format(received_size))
    verified = True

    if received_size > 0:
        for i, value in enumerate(received_buffer):
            if value != buffer[i]:
                print('Mismatch at {:d}: {:02X} instead of {:02X}'.format(i, value, buffer[i]))
                verified = False
                break
    else:
        verified = False

    if verified:
        print('Buffers match')
    else:
        print('Buffers mismatch')
        sys.exit(errno.ECOMM)

if __name__ == '__main__':
    main()
