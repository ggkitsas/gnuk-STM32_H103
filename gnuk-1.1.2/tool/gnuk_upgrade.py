#! /usr/bin/python

"""
gnuk_upgrade.py - a tool to upgrade firmware of Gnuk Token

Copyright (C) 2012 Free Software Initiative of Japan
Author: NIIBE Yutaka <gniibe@fsij.org>

This file is a part of Gnuk, a GnuPG USB Token implementation.

Gnuk is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Gnuk is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

from struct import *
import sys, time, os, binascii, string

# INPUT: binary files (regnual_image, upgrade_firmware_image)

# Assume only single CCID device is attached to computer, and it's Gnuk Token

import usb

# USB class, subclass, protocol
CCID_CLASS = 0x0B
CCID_SUBCLASS = 0x00
CCID_PROTOCOL_0 = 0x00

def icc_compose(msg_type, data_len, slot, seq, param, data):
    return pack('<BiBBBH', msg_type, data_len, slot, seq, 0, param) + data

def iso7816_compose(ins, p1, p2, data, cls=0x00):
    data_len = len(data)
    if data_len == 0:
        return pack('>BBBB', cls, ins, p1, p2)
    else:
        return pack('>BBBBB', cls, ins, p1, p2, data_len) + data

class regnual(object):
    def __init__(self, dev):
        conf = dev.configurations[0]
        intf_alt = conf.interfaces[0]
        intf = intf_alt[0]
        if intf.interfaceClass != 0xff:
            raise ValueError, "Wrong interface class"
        self.__devhandle = dev.open()
        try:
            self.__devhandle.setConfiguration(conf)
        except:
            pass
        self.__devhandle.claimInterface(intf)
        self.__devhandle.setAltInterface(0)

    def mem_info(self):
        mem = self.__devhandle.controlMsg(requestType = 0xc0, request = 0,
                                          value = 0, index = 0, buffer = 8,
                                          timeout = 10000)
        start = ((mem[3]*256 + mem[2])*256 + mem[1])*256 + mem[0]
        end = ((mem[7]*256 + mem[6])*256 + mem[5])*256 + mem[4]
        return (start, end)

    def download(self, start, data):
        addr = start
        addr_end = (start + len(data)) & 0xffffff00
        i = (addr - 0x08000000) / 0x100
        j = 0
        print "start %08x" % addr
        print "end   %08x" % addr_end
        while addr < addr_end:
            print "# %08x: %d: %d : %d" % (addr, i, j, 256)
            self.__devhandle.controlMsg(requestType = 0x40, request = 1,
                                        value = 0, index = 0,
                                        buffer = data[j*256:j*256+256],
                                        timeout = 10000)
            crc32code = crc32(data[j*256:j*256+256])
            res = self.__devhandle.controlMsg(requestType = 0xc0, request = 2,
                                              value = 0, index = 0, buffer = 4,
                                              timeout = 10000)
            r_value = ((res[3]*256 + res[2])*256 + res[1])*256 + res[0]
            if (crc32code ^ r_value) != 0xffffffff:
                print "failure"
            self.__devhandle.controlMsg(requestType = 0x40, request = 3,
                                        value = i, index = 0,
                                        buffer = None,
                                        timeout = 10000)
            time.sleep(0.010)
            res = self.__devhandle.controlMsg(requestType = 0xc0, request = 2,
                                              value = 0, index = 0, buffer = 4,
                                              timeout = 10000)
            r_value = ((res[3]*256 + res[2])*256 + res[1])*256 + res[0]
            if r_value == 0:
                print "failure"
            i = i+1
            j = j+1
            addr = addr + 256
        residue = len(data) % 256
        if residue != 0:
            print "# %08x: %d : %d" % (addr, i, residue)
            self.__devhandle.controlMsg(requestType = 0x40, request = 1,
                                        value = 0, index = 0,
                                        buffer = data[j*256:],
                                        timeout = 10000)
            crc32code = crc32(data[j*256:].ljust(256,chr(255)))
            res = self.__devhandle.controlMsg(requestType = 0xc0, request = 2,
                                              value = 0, index = 0, buffer = 4,
                                              timeout = 10000)
            r_value = ((res[3]*256 + res[2])*256 + res[1])*256 + res[0]
            if (crc32code ^ r_value) != 0xffffffff:
                print "failure"
            self.__devhandle.controlMsg(requestType = 0x40, request = 3,
                                        value = i, index = 0,
                                        buffer = None,
                                        timeout = 10000)
            time.sleep(0.010)
            res = self.__devhandle.controlMsg(requestType = 0xc0, request = 2,
                                              value = 0, index = 0, buffer = 4,
                                              timeout = 10000)
            r_value = ((res[3]*256 + res[2])*256 + res[1])*256 + res[0]
            if r_value == 0:
                print "failure"

    def protect(self):
        self.__devhandle.controlMsg(requestType = 0x40, request = 4,
                                    value = 0, index = 0, buffer = None,
                                    timeout = 10000)
        time.sleep(0.100)
        res = self.__devhandle.controlMsg(requestType = 0xc0, request = 2,
                                          value = 0, index = 0, buffer = 4,
                                          timeout = 10000)
        r_value = ((res[3]*256 + res[2])*256 + res[1])*256 + res[0]
        if r_value == 0:
            print "protection failure"

    def finish(self):
        self.__devhandle.controlMsg(requestType = 0x40, request = 5,
                                    value = 0, index = 0, buffer = None,
                                    timeout = 10000)

    def reset_device(self):
        try:
            self.__devhandle.reset()
        except:
            pass

# This class only supports Gnuk (for now) 
class gnuk_token(object):
    def __init__(self, device, configuration, interface):
        """
        __init__(device, configuration, interface) -> None
        Initialize the device.
        device: usb.Device object.
        configuration: configuration number.
        interface: usb.Interface object representing the interface and altenate setting.
        """
        if interface.interfaceClass != CCID_CLASS:
            raise ValueError, "Wrong interface class"
        if interface.interfaceSubClass != CCID_SUBCLASS:
            raise ValueError, "Wrong interface sub class"
        self.__devhandle = device.open()
        try:
            self.__devhandle.setConfiguration(configuration)
        except:
            pass
        self.__devhandle.claimInterface(interface)
        self.__devhandle.setAltInterface(0)

        self.__intf = interface.interfaceNumber
        self.__alt = interface.alternateSetting
        self.__conf = configuration

        self.__bulkout = 1
        self.__bulkin  = 0x81

        self.__timeout = 10000
        self.__seq = 0

    def reset_device(self):
        try:
            self.__devhandle.reset()
        except:
            pass

    def stop_gnuk(self):
        self.__devhandle.releaseInterface()
        self.__devhandle.setConfiguration(0)
        return

    def mem_info(self):
        mem = self.__devhandle.controlMsg(requestType = 0xc0, request = 0,
                                          value = 0, index = 0, buffer = 8,
                                          timeout = 10)
        start = ((mem[3]*256 + mem[2])*256 + mem[1])*256 + mem[0]
        end = ((mem[7]*256 + mem[6])*256 + mem[5])*256 + mem[4]
        return (start, end)

    def download(self, start, data):
        addr = start
        addr_end = (start + len(data)) & 0xffffff00
        i = (addr - 0x20000000) / 0x100
        j = 0
        print "start %08x" % addr
        print "end   %08x" % addr_end
        while addr < addr_end:
            print "# %08x: %d : %d" % (addr, i, 256)
            self.__devhandle.controlMsg(requestType = 0x40, request = 1,
                                        value = i, index = 0,
                                        buffer = data[j*256:j*256+256],
                                        timeout = 10)
            i = i+1
            j = j+1
            addr = addr + 256
        residue = len(data) % 256
        if residue != 0:
            print "# %08x: %d : %d" % (addr, i, residue)
            self.__devhandle.controlMsg(requestType = 0x40, request = 1,
                                        value = i, index = 0,
                                        buffer = data[j*256:],
                                        timeout = 10)

    def execute(self, last_addr):
        i = (last_addr - 0x20000000) / 0x100
        o = (last_addr - 0x20000000) % 0x100
        self.__devhandle.controlMsg(requestType = 0x40, request = 2,
                                    value = i, index = o, buffer = None,
                                    timeout = 10)

    def icc_get_result(self):
        msg = self.__devhandle.bulkRead(self.__bulkin, 1024, self.__timeout)
        if len(msg) < 10:
            raise ValueError, "icc_get_result"
        msg_type = msg[0]
        data_len = msg[1] + (msg[2]<<8) + (msg[3]<<16) + (msg[4]<<24)
        slot = msg[5]
        seq = msg[6]
        status = msg[7]
        error = msg[8]
        chain = msg[9]
        data = msg[10:]
        # XXX: check msg_type, data_len, slot, seq, error
        return (status, chain, data)

    def icc_get_status(self):
        msg = icc_compose(0x65, 0, 0, self.__seq, 0, "")
        self.__devhandle.bulkWrite(self.__bulkout, msg, self.__timeout)
        self.__seq += 1
        status, chain, data = self.icc_get_result()
        # XXX: check chain, data
        return status

    def icc_power_on(self):
        msg = icc_compose(0x62, 0, 0, self.__seq, 0, "")
        self.__devhandle.bulkWrite(self.__bulkout, msg, self.__timeout)
        self.__seq += 1
        status, chain, data = self.icc_get_result()
        # XXX: check status, chain
        return data             # ATR

    def icc_power_off(self):
        msg = icc_compose(0x63, 0, 0, self.__seq, 0, "")
        self.__devhandle.bulkWrite(self.__bulkout, msg, self.__timeout)
        self.__seq += 1
        status, chain, data = self.icc_get_result()
        # XXX: check chain, data
        return status

    def icc_send_data_block(self, data):
        msg = icc_compose(0x6f, len(data), 0, self.__seq, 0, data)
        self.__devhandle.bulkWrite(self.__bulkout, msg, self.__timeout)
        self.__seq += 1
        return self.icc_get_result()

    def icc_send_cmd(self, data):
        status, chain, data_rcv = self.icc_send_data_block(data)
        if chain == 0:
            return data_rcv
        elif chain == 1:
            d = data_rcv
            while True:
                msg = icc_compose(0x6f, 0, 0, self.__seq, 0x10, "")
                self.__devhandle.bulkWrite(self.__bulkout, msg, self.__timeout)
                self.__seq += 1
                status, chain, data_rcv = self.icc_get_result()
                # XXX: check status
                d += data_rcv
                if chain == 2:
                    break
                elif chain == 3:
                    continue
                else:
                    raise ValueError, "icc_send_cmd chain"
            return d
        else:
            raise ValueError, "icc_send_cmd"

    def cmd_get_response(self, expected_len):
        cmd_data = iso7816_compose(0xc0, 0x00, 0x00, '') + pack('>B', expected_len)
        response = self.icc_send_cmd(cmd_data)
        return response[:-2]

    def cmd_verify(self, who, passwd):
        cmd_data = iso7816_compose(0x20, 0x00, 0x80+who, passwd)
        sw = self.icc_send_cmd(cmd_data)
        if len(sw) != 2:
            raise ValueError, sw
        if not (sw[0] == 0x90 and sw[1] == 0x00):
            raise ValueError, sw

    def cmd_select_openpgp(self):
        cmd_data = iso7816_compose(0xa4, 0x04, 0x0c, "\xD2\x76\x00\x01\x24\x01")
        sw = self.icc_send_cmd(cmd_data)
        if len(sw) != 2:
            raise ValueError, sw
        if not (sw[0] == 0x90 and sw[1] == 0x00):
            raise ValueError, ("%02x%02x" % (sw[0], sw[1]))

    def cmd_external_authenticate(self, signed):
        cmd_data = iso7816_compose(0x82, 0x00, 0x00, signed[0:128], cls=0x10)
        sw = self.icc_send_cmd(cmd_data)
        if len(sw) != 2:
            raise ValueError, sw
        if not (sw[0] == 0x90 and sw[1] == 0x00):
            raise ValueError, ("%02x%02x" % (sw[0], sw[1]))
        cmd_data = iso7816_compose(0x82, 0x00, 0x00, signed[128:])
        sw = self.icc_send_cmd(cmd_data)
        if len(sw) != 2:
            raise ValueError, sw
        if not (sw[0] == 0x90 and sw[1] == 0x00):
            raise ValueError, ("%02x%02x" % (sw[0], sw[1]))

    def cmd_get_challenge(self):
        cmd_data = iso7816_compose(0x84, 0x00, 0x00, '')
        sw = self.icc_send_cmd(cmd_data)
        if len(sw) != 2:
            raise ValueError, sw
        if sw[0] != 0x61:
            raise ValueError, ("%02x%02x" % (sw[0], sw[1]))
        return self.cmd_get_response(sw[1])

def compare(data_original, data_in_device):
    i = 0 
    for d in data_original:
        if ord(d) != data_in_device[i]:
            raise ValueError, "verify failed at %08x" % i
        i += 1

def ccid_devices():
    busses = usb.busses()
    for bus in busses:
        devices = bus.devices
        for dev in devices:
            for config in dev.configurations:
                for intf in config.interfaces:
                    for alt in intf:
                        if alt.interfaceClass == CCID_CLASS and \
                                alt.interfaceSubClass == CCID_SUBCLASS and \
                                alt.interfaceProtocol == CCID_PROTOCOL_0:
                            yield dev, config, alt

USB_VENDOR_FSIJ=0x234b
USB_PRODUCT_GNUK=0x0000

def gnuk_devices():
    busses = usb.busses()
    for bus in busses:
        devices = bus.devices
        for dev in devices:
            if dev.idVendor != USB_VENDOR_FSIJ:
                continue
            if dev.idProduct != USB_PRODUCT_GNUK:
                continue
            yield dev

def to_string(t):
    result = ""
    for c in t:
        result += chr(c)
    return result

from subprocess import check_output

SHA256_OID_PREFIX="3031300d060960864801650304020105000420"

# When user specify KEYGRIP, use it.  Or else, connect to SCD directly.
def gpg_sign(keygrip, hash):
    if keygrip:
        result = check_output(["gpg-connect-agent",
                               "SIGKEY %s" % keygrip,
                               "SETHASH --hash=sha256 %s" % hash,
                               "PKSIGN --hash=sha256", "/bye"])
    else:
        result = check_output(["gpg-connect-agent",
                               "SCD SETDATA " + SHA256_OID_PREFIX + hash,
                               "SCD PKAUTH OPENPGP.3",
                               "/bye"])
    signed = ""
    while True:
        i = result.find('%')
        if i < 0:
            signed += result
            break
        hex_str = result[i+1:i+3]
        signed += result[0:i]
        signed += chr(int(hex_str,16))
        result = result[i+3:]

    if keygrip:
        pos = signed.index("D (7:sig-val(3:rsa(1:s256:") + 26
        signed = signed[pos:-7]
    else:
        pos = signed.index("D ") + 2
        signed = signed[pos:-4]     # \nOK\n
    if len(signed) != 256:
        raise ValueError, binascii.hexlify(signed)
    return signed

def UNSIGNED(n):
    return n & 0xffffffff

def crc32(bytestr):
    crc = binascii.crc32(bytestr)
    return UNSIGNED(crc)

def main(keygrip, data_regnual, data_upgrade):
    l = len(data_regnual)
    if (l & 0x03) != 0:
        data_regnual = data_regnual.ljust(l + 4 - (l & 0x03), chr(0))
    crc32code = crc32(data_regnual)
    print "CRC32: %04x\n" % crc32code
    data_regnual += pack('<I', crc32code)
    for (dev, config, intf) in ccid_devices():
        try:
            icc = gnuk_token(dev, config, intf)
            print "Device: ", dev.filename
            print "Configuration: ", config.value
            print "Interface: ", intf.interfaceNumber
            break
        except:
            icc = None
    if icc.icc_get_status() == 2:
        raise ValueError, "No ICC present"
    elif icc.icc_get_status() == 1:
        icc.icc_power_on()
    icc.cmd_select_openpgp()
    challenge = icc.cmd_get_challenge()
    signed = gpg_sign(keygrip, binascii.hexlify(to_string(challenge)))
    icc.cmd_external_authenticate(signed)
    icc.stop_gnuk()
    mem_info = icc.mem_info()
    print "%08x:%08x" % mem_info
    print "Downloading flash upgrade program..."
    icc.download(mem_info[0], data_regnual)
    print "Run flash upgrade program..."
    icc.execute(mem_info[0] + len(data_regnual) - 4)
    #
    time.sleep(3)
    icc.reset_device()
    del icc
    icc = None
    #
    print "Wait 3 seconds..."
    time.sleep(3)
    # Then, send upgrade program...
    reg = None
    for dev in gnuk_devices():
        try:
            reg = regnual(dev)
            print "Device: ", dev.filename
            break
        except:
            pass
    mem_info = reg.mem_info()
    print "%08x:%08x" % mem_info
    print "Downloading the program"
    reg.download(mem_info[0], data_upgrade)
    reg.protect()
    reg.finish()
    reg.reset_device()
    return 0


if __name__ == '__main__':
    keygrip = None
    if sys.argv[1] == '-k':
        sys.argv.pop(1)
        keygrip = sys.argv[1]
        sys.argv.pop(1)
    filename_regnual = sys.argv[1]
    filename_upgrade = sys.argv[2]
    f = open(filename_regnual)
    data_regnual = f.read()
    f.close()
    print "%s: %d" % (filename_regnual, len(data_regnual))
    f = open(filename_upgrade)
    data_upgrade = f.read()
    f.close()
    print "%s: %d" % (filename_upgrade, len(data_upgrade))
    main(keygrip, data_regnual, data_upgrade[4096:])
