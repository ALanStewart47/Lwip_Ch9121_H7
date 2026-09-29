#!/usr/bin/env python3
"""Probe an H743 CH9121-compatible UDP configuration endpoint."""

import argparse
import ipaddress
import socket
import sys
import time


FLAG = b"CH9121_CFG_FLAG\0"
PACKET_SIZE = 285
HEADER_SIZE = 30
DATA_SIZE = 255
CONFIG_SIZE = 204
DEVICE_PORT = 50000
CLIENT_PORT = 60000
NAME = 5
IP = 32
GATEWAY = 36
MASK = 40
DHCP = 44
PORT1 = 139


class ProbeError(RuntimeError):
    pass


def parse_mac(value):
    compact = value.replace(":", "").replace("-", "").replace(".", "")
    if len(compact) != 12:
        raise argparse.ArgumentTypeError("MAC address must contain six bytes")
    try:
        return bytes.fromhex(compact)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("invalid MAC address") from exc


def parse_ipv4(value):
    try:
        return ipaddress.IPv4Address(value).packed
    except ipaddress.AddressValueError as exc:
        raise argparse.ArgumentTypeError(str(exc)) from exc


def mac_text(value):
    return ":".join("%02X" % byte for byte in value)


def ipv4_text(value):
    return str(ipaddress.IPv4Address(bytes(value)))


def encode_name(name):
    try:
        encoded = name.encode("ascii")
    except UnicodeEncodeError as exc:
        raise ProbeError("name must contain printable ASCII characters") from exc
    if not 1 <= len(encoded) <= 20 or any(byte < 0x20 or byte > 0x7E for byte in encoded):
        raise ProbeError("name must contain 1 to 20 printable ASCII characters")
    field = bytearray(21)
    field[:len(encoded)] = encoded
    return bytes(field)


def make_request(command, target_mac, pc_mac, length=0, data=None):
    packet = bytearray(PACKET_SIZE)
    packet[0:16] = FLAG
    packet[16] = command
    packet[17:23] = target_mac
    packet[23:29] = pc_mac
    packet[29] = length
    if data is not None:
        if len(data) != DATA_SIZE:
            raise ProbeError("configuration data must be 255 bytes")
        packet[HEADER_SIZE:] = data
    return bytes(packet)


def config_name(data):
    field = data[NAME:NAME + 21]
    end = field.find(b"\0")
    if end < 0:
        raise ProbeError("GET response has an unterminated device name")
    return field[:end].decode("ascii", errors="replace")


def comparable_config(data):
    comparable = bytearray(data)
    if comparable[DHCP] == 1:
        comparable[IP:DHCP] = bytes(DHCP - IP)
    return bytes(comparable)


def print_config(packet):
    data = packet[HEADER_SIZE:]
    print("Device MAC : %s" % mac_text(packet[17:23]))
    print("Name       : %r" % config_name(data))
    print("DHCP       : %s" % ("on" if data[DHCP] else "off"))
    print("IPv4       : %s" % ipv4_text(data[IP:IP + 4]))
    print("Mask       : %s" % ipv4_text(data[MASK:MASK + 4]))
    print("Gateway    : %s" % ipv4_text(data[GATEWAY:GATEWAY + 4]))
    print("Port1      : disabled placeholder, 19200/8N1")


class Client:
    def __init__(self, interface_ip, pc_mac, broadcast):
        self.pc_mac = pc_mac
        self.broadcast = broadcast
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.sock.bind((interface_ip, CLIENT_PORT))

    def close(self):
        self.sock.close()

    def send(self, packet):
        self.sock.sendto(packet, (self.broadcast, DEVICE_PORT))

    def receive(self, command, target_mac=None, timeout=1.0):
        return self.receive_matching(command, target_mac, timeout)

    def receive_matching(self, command=None, target_mac=None, timeout=1.0):
        deadline = time.monotonic() + timeout
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return None
            self.sock.settimeout(min(0.1, remaining))
            try:
                packet, source = self.sock.recvfrom(2048)
            except socket.timeout:
                continue
            if source[1] != DEVICE_PORT or len(packet) != PACKET_SIZE:
                continue
            if packet[0:16] != FLAG or (command is not None and packet[16] != command):
                continue
            if target_mac is not None and packet[17:23] != target_mac:
                continue
            if packet[23:29] not in (self.pc_mac, bytes(6)):
                continue
            return packet

    def scan(self, window=1.5):
        self.send(make_request(0x04, bytes(6), self.pc_mac))
        deadline = time.monotonic() + window
        devices = {}
        while time.monotonic() < deadline:
            packet = self.receive(0x84, timeout=min(0.1, deadline - time.monotonic()))
            if packet is None:
                continue
            data = packet[HEADER_SIZE:]
            length = packet[29]
            if length < 6 or length > 25 or data[length - 1] != 0:
                continue
            version = data[length]
            if version != 0x47:
                print("Ignoring %s: version is 0x%02X, expected 0x47" %
                      (mac_text(packet[17:23]), version))
                continue
            name = data[4:length - 1].decode("ascii", errors="replace")
            devices[packet[17:23]] = (bytes(data[0:4]), name)
        return devices

    def get_config(self, target_mac, attempts=3):
        request = make_request(0x02, target_mac, self.pc_mac)
        for _ in range(attempts):
            self.send(request)
            response = self.receive(0x82, target_mac, timeout=0.8)
            if response is not None and response[29] == 0xCC:
                return response
        raise ProbeError("no valid GET response from %s" % mac_text(target_mac))

    def set_config(self, target_mac, data):
        request = make_request(0x01, target_mac, self.pc_mac, 0xCC, data)
        self.send(request)  # SET is sent once; a missing ACK is not retried.
        response = self.receive(0x81, target_mac, timeout=1.0)
        if response is None or response[29] != 0:
            raise ProbeError("SET ACK was not received")
        if response[HEADER_SIZE:HEADER_SIZE + CONFIG_SIZE] != data[:CONFIG_SIZE]:
            raise ProbeError("SET ACK did not echo the accepted configuration")
        return response

    def expect_no_response(self, label, request, wait=0.3):
        self.sock.setblocking(False)
        try:
            while True:
                self.sock.recvfrom(2048)
        except BlockingIOError:
            pass
        finally:
            self.sock.setblocking(True)
        self.send(request)
        packet = self.receive_matching(timeout=wait)
        if packet is not None:
            raise ProbeError("%s unexpectedly produced command 0x%02X from %s" %
                             (label, packet[16], mac_text(packet[17:23])))
        print("PASS: %s is ignored" % label)

    def set_config_expect_nak(self, target_mac, data, label):
        packet = make_request(0x01, target_mac, self.pc_mac, 0xCC, data)
        self.send(packet)
        response = self.receive(0xC1, target_mac, timeout=1.0)
        if response is None or response[29] != 0:
            raise ProbeError("%s did not receive the expected C1 response" % label)
        if response[HEADER_SIZE:] != data:
            raise ProbeError("%s C1 did not echo the rejected configuration" % label)
        print("PASS: %s returns C1" % label)


def add_connection_args(parser):
    parser.add_argument("--interface-ip", required=True,
                        help="IPv4 address of the selected PC network interface")
    parser.add_argument("--pc-mac", required=True, type=parse_mac,
                        help="MAC address of that same interface")
    parser.add_argument("--broadcast", default="255.255.255.255",
                        help="broadcast destination (default: 255.255.255.255)")


def add_target_arg(parser):
    parser.add_argument("--target", required=True, type=parse_mac,
                        help="device MAC from a search response")


def run_scan(client, args):
    devices = client.scan(args.window)
    for mac, (ip, name) in devices.items():
        print("%s  %-21s %s  version=47" % (mac_text(mac), name, ipv4_text(ip)))
    print("%u device(s) discovered" % len(devices))
    if not devices:
        raise ProbeError("no compatible device found")


def run_get(client, args):
    packet = client.get_config(args.target)
    print_config(packet)


def run_set(client, args):
    options = (args.name, args.dhcp, args.ip, args.mask, args.gateway)
    if all(value is None for value in options):
        raise ProbeError("specify at least one of --name/--dhcp/--ip/--mask/--gateway")

    baseline = client.get_config(args.target)
    data = bytearray(baseline[HEADER_SIZE:])
    if args.name is not None:
        data[NAME:NAME + 21] = encode_name(args.name)
    if args.dhcp is not None:
        data[DHCP] = args.dhcp
    for option, offset, label in ((args.ip, IP, "IP"), (args.gateway, GATEWAY, "gateway"),
                                  (args.mask, MASK, "mask")):
        if option is not None:
            if data[DHCP] == 1:
                raise ProbeError("do not edit %s while enabling DHCP" % label)
            data[offset:offset + 4] = option

    if data[DHCP] == 0:
        ip = ipaddress.IPv4Address(bytes(data[IP:IP + 4]))
        mask = ipaddress.IPv4Address(bytes(data[MASK:MASK + 4]))
        gateway = ipaddress.IPv4Address(bytes(data[GATEWAY:GATEWAY + 4]))
        if ip.is_unspecified or ip.is_multicast or ip.is_loopback:
            raise ProbeError("static IP must be a unicast host address")
        if mask.is_unspecified or mask == ipaddress.IPv4Address("255.255.255.255"):
            raise ProbeError("static mask must leave both network and host bits")
        try:
            network = ipaddress.IPv4Network((str(ip), str(mask)), strict=False)
        except ValueError as exc:
            raise ProbeError("static mask must be contiguous") from exc
        if ip in (network.network_address, network.broadcast_address):
            raise ProbeError("static IP cannot be the network or broadcast address")
        if not gateway.is_unspecified and (gateway == ip or gateway not in network or gateway in
                                           (network.network_address, network.broadcast_address)):
            raise ProbeError("gateway must be a host in the same subnet or 0.0.0.0")

    print("Sending one SET to %s; waiting for ACK and readback" % mac_text(args.target))
    client.set_config(args.target, data)
    deadline = time.monotonic() + 15.0
    last_error = None
    while time.monotonic() < deadline:
        try:
            devices = client.scan(0.7)
            if args.target not in devices:
                continue
            result = client.get_config(args.target, attempts=1)
            actual = result[HEADER_SIZE:]
            fields = [(NAME, NAME + 21, "name"), (DHCP, DHCP + 1, "DHCP")]
            if data[DHCP] == 0:
                fields.extend(((IP, IP + 4, "IP"), (MASK, MASK + 4, "mask"),
                               (GATEWAY, GATEWAY + 4, "gateway")))
            for start, end, name in fields:
                if actual[start:end] != data[start:end]:
                    raise ProbeError("readback mismatch in %s" % name)
            print("PASS: SET verified by search and GET readback")
            print_config(result)
            return
        except ProbeError as exc:
            last_error = exc
            time.sleep(0.2)
    raise ProbeError("write was sent, but readback was not verified: %s" % last_error)


def run_negative(client, args):
    baseline = client.get_config(args.target)
    data = bytearray(baseline[HEADER_SIZE:])

    wrong_length = make_request(0x02, args.target, client.pc_mac)[:-1]
    client.expect_no_response("284-byte packet", wrong_length)

    bad_flag = bytearray(make_request(0x02, args.target, client.pc_mac))
    bad_flag[0] ^= 0x20
    client.expect_no_response("incorrect protocol flag", bytes(bad_flag))

    wrong_target = make_request(0x02, bytes((args.target[0] ^ 2,)) + args.target[1:],
                                client.pc_mac)
    client.expect_no_response("non-target MAC", wrong_target)

    data[PORT1 + 12] ^= 1
    client.set_config_expect_nak(args.target, data, "read-only port setting")

    data = bytearray(baseline[HEADER_SIZE:])
    data[DHCP] = 0
    data[IP:IP + 4] = bytes(4)
    client.set_config_expect_nak(args.target, data, "invalid static IP")

    data = bytearray(baseline[HEADER_SIZE:])
    data[NAME:NAME + 21] = b"A" * 21
    client.set_config_expect_nak(args.target, data, "unterminated 21-byte name")

    data = bytearray(baseline[HEADER_SIZE:])
    data[DHCP] = 0
    data[IP:IP + 4] = bytes((192, 168, 1, 30))
    data[MASK:MASK + 4] = bytes((255, 0, 255, 0))
    data[GATEWAY:GATEWAY + 4] = bytes((192, 168, 1, 1))
    client.set_config_expect_nak(args.target, data, "non-contiguous subnet mask")

    data = bytearray(baseline[HEADER_SIZE:])
    data[DHCP] = 2
    client.set_config_expect_nak(args.target, data, "invalid DHCP value")

    data = bytearray(baseline[HEADER_SIZE:])
    data[DHCP] = 0
    data[IP:IP + 4] = bytes((192, 168, 1, 30))
    data[MASK:MASK + 4] = bytes((255, 255, 255, 0))
    data[GATEWAY:GATEWAY + 4] = bytes((192, 168, 2, 1))
    client.set_config_expect_nak(args.target, data, "gateway outside the device subnet")

    data[GATEWAY:GATEWAY + 4] = data[IP:IP + 4]
    client.set_config_expect_nak(args.target, data, "gateway equal to the device IP")

    unchanged = client.get_config(args.target)[HEADER_SIZE:]
    if comparable_config(unchanged) != comparable_config(baseline[HEADER_SIZE:]):
        raise ProbeError("a rejected SET changed the configuration")
    print("PASS: rejected SETs leave the full 255-byte configuration unchanged")

    duplicate = make_request(0x02, args.target, client.pc_mac)
    client.send(duplicate)
    client.send(duplicate)
    first = client.receive(0x82, args.target, timeout=1.0)
    second = client.receive(0x82, args.target, timeout=1.0)
    if first is None or second is None or first != second:
        raise ProbeError("duplicate GET requests did not produce two matching responses")
    print("PASS: duplicate GET requests each produce a valid response")


def set_name_and_wait(client, target_mac, base_packet, name):
    desired = bytearray(base_packet[HEADER_SIZE:])
    desired[NAME:NAME + 21] = encode_name(name)
    client.set_config(target_mac, desired)

    deadline = time.monotonic() + 5.0
    while time.monotonic() < deadline:
        try:
            response = client.get_config(target_mac, attempts=1)
        except ProbeError:
            time.sleep(0.2)
            continue
        if response[HEADER_SIZE + NAME:HEADER_SIZE + NAME + 21] == desired[NAME:NAME + 21]:
            return response
        time.sleep(0.2)
    raise ProbeError("device did not read back boundary name %r" % name)


def run_boundary(client, args):
    original_packet = client.get_config(args.target)
    original_data = original_packet[HEADER_SIZE:]
    original_name = original_data[NAME:NAME + 21]
    terminator = original_name.find(b"\0")
    if terminator < 1:
        raise ProbeError("current device name cannot be restored safely")
    restore_name = original_name[:terminator].decode("ascii")
    attempted_set = False

    try:
        for name in ("A", "ABCDEFGHIJKLMNOPQRST"):
            current = client.get_config(args.target)
            attempted_set = True
            set_name_and_wait(client, args.target, current, name)
            print("PASS: device accepted a %u-character name" % len(name))
    finally:
        if attempted_set:
            try:
                current = client.get_config(args.target)
                set_name_and_wait(client, args.target, current, restore_name)
                print("PASS: restored original device name %r" % restore_name)
            except (ProbeError, OSError) as exc:
                raise ProbeError("failed to restore original device name: %s" % exc) from exc

    print("PASS: invalid packets were rejected without applying configuration")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)

    scan_parser = commands.add_parser("scan", help="broadcast search and list devices")
    add_connection_args(scan_parser)
    scan_parser.add_argument("--window", type=float, default=1.5)
    scan_parser.set_defaults(run=run_scan)

    get_parser = commands.add_parser("get", help="read a device configuration")
    add_connection_args(get_parser)
    add_target_arg(get_parser)
    get_parser.set_defaults(run=run_get)

    set_parser = commands.add_parser("set", help="write selected fields once and verify by readback")
    add_connection_args(set_parser)
    add_target_arg(set_parser)
    set_parser.add_argument("--name")
    set_parser.add_argument("--dhcp", type=int, choices=(0, 1))
    set_parser.add_argument("--ip", type=parse_ipv4)
    set_parser.add_argument("--mask", type=parse_ipv4)
    set_parser.add_argument("--gateway", type=parse_ipv4)
    set_parser.set_defaults(run=run_set)

    negative_parser = commands.add_parser("negative", help="check malformed and unsupported SET cases")
    add_connection_args(negative_parser)
    add_target_arg(negative_parser)
    negative_parser.set_defaults(run=run_negative)

    boundary_parser = commands.add_parser("boundary", help="exercise 1- and 20-character device names, then restore")
    add_connection_args(boundary_parser)
    add_target_arg(boundary_parser)
    boundary_parser.set_defaults(run=run_boundary)

    args = parser.parse_args()
    try:
        client = Client(args.interface_ip, args.pc_mac, args.broadcast)
    except OSError as exc:
        print("Cannot bind %s:%u: %s" % (args.interface_ip, CLIENT_PORT, exc), file=sys.stderr)
        print("Close NetModuleConfig_V2.04 before running this probe.", file=sys.stderr)
        return 2

    try:
        args.run(client, args)
    except (ProbeError, OSError, UnicodeError) as exc:
        print("ERROR: %s" % exc, file=sys.stderr)
        return 1
    finally:
        client.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
