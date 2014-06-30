#! /usr/bin/python

"""
gnuk_put_binary.py - a tool to put binary to Gnuk Token
This tool is for importing certificate, writing serial number, etc.

Copyright (C) 2011, 2012 Free Software Initiative of Japan
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

import sys, os, binascii, string

# INPUT: binary file

# Assume only single CCID device is attached to computer and it's Gnuk Token

from smartcard.CardType import AnyCardType
from smartcard.CardRequest import CardRequest
from smartcard.util import toHexString

def s2l(s):
    return [ ord(c) for c in s ]

class GnukToken(object):
    def __init__(self):
        cardtype = AnyCardType()
        cardrequest = CardRequest(timeout=1, cardType=cardtype)
        cardservice = cardrequest.waitforcard()
        self.connection = cardservice.connection

    def cmd_get_response(self, expected_len):
        result = []
        while True:
            apdu = [0x00, 0xc0, 0x00, 0x00, expected_len]
            response, sw1, sw2 = self.connection.transmit(apdu)
            result += response
            if sw1 == 0x90 and sw2 == 0x00:
                return result
            elif sw1 != 0x61: 
                raise ValueError, ("%02x%02x" % (sw1, sw2))
            else:
                expected_len = sw2

    def cmd_verify(self, who, passwd):
        apdu = [0x00, 0x20, 0x00, 0x80+who, len(passwd)] + s2l(passwd)
        response, sw1, sw2 = self.connection.transmit(apdu)
        if not (sw1 == 0x90 and sw2 == 0x00):
            raise ValueError, ("%02x%02x" % (sw1, sw2))

    def cmd_read_binary(self, fileid):
        apdu = [0x00, 0xb0, 0x80+fileid, 0x00] 
        response, sw1, sw2 = self.connection.transmit(apdu)
        if sw1 == 0x61:
            response = self.cmd_get_response(sw2)
        elif not (sw1 == 0x90 and sw2 == 0x00):
            raise ValueError, ("%02x%02x" % (sw1, sw2))
        return response

    def cmd_write_binary(self, fileid, data, is_update):
        count = 0
        data_len = len(data)
        if is_update:
            ins = 0xd6
        else:
            ins = 0xd0
        while count*256 < data_len:
            if count == 0:
                d = data[:256]
                if len(d) <= 255:
                    apdu = [0x00, ins, 0x80+fileid, 0x00, len(d)] + s2l(d)
                else:
                    apdu0 = [0x10, ins, 0x80+fileid, 0x00, 255] + s2l(d[:255])
                    response, sw1, sw2 = self.connection.transmit(apdu0)
                    apdu = [0x00, ins, 0x80+fileid, 0x00, 1 ] + s2l(d[255:])
            else:
                d = data[256*count:256*(count+1)]
                if len(d) <= 255:
                    apdu = [0x00, ins, count, 0x00, len(d)] + s2l(d)
                else:
                    apdu0 = [0x10, ins, count, 0x00, 255] + s2l(d[:255])
                    response, sw1, sw2 = self.connection.transmit(apdu0)
                    apdu = [0x00, ins, count, 0x00, 1] + s2l(d[255:])
            response, sw1, sw2 = self.connection.transmit(apdu)
            if not (sw1 == 0x90 and sw2 == 0x00):
                if is_update:
                    raise ValueError, ("update failure: %02x%02x" % (sw1, sw2))
                else:
                    raise ValueError, ("write failure: %02x%02x" % (sw1, sw2))
            count += 1

    def cmd_select_openpgp(self):
        apdu = [0x00, 0xa4, 0x04, 0x0c, 6, 0xd2, 0x76, 0x00, 0x01, 0x24, 0x01]
        response, sw1, sw2 = self.connection.transmit(apdu)
        if sw1 == 0x61:
            response = self.cmd_get_response(sw2)
        elif not (sw1 == 0x90 and sw2 == 0x00):
            raise ValueError, ("%02x%02x" % (sw1, sw2))

    def cmd_get_data(self, tagh, tagl):
        apdu = [0x00, 0xca, tagh, tagl]
        response, sw1, sw2 = self.connection.transmit(apdu)
        if sw1 == 0x61:
            response = self.cmd_get_response(sw2)
        elif not (sw1 == 0x90 and sw2 == 0x00):
            raise ValueError, ("%02x%02x" % (sw1, sw2))
        return response

def compare(data_original, data_in_device):
    i = 0 
    for d in data_original:
        if ord(d) != data_in_device[i]:
            raise ValueError, "verify failed at %08x" % i
        i += 1

DEFAULT_PW3 = "12345678"
BY_ADMIN = 3

def main(fileid, is_update, data, passwd):
    gnuk = GnukToken()

    gnuk.connection.connect()
    print "Token:", gnuk.connection.getReader()
    print "ATR:", toHexString( gnuk.connection.getATR() )

    gnuk.cmd_verify(BY_ADMIN, passwd)
    gnuk.cmd_write_binary(fileid, data, is_update)
    gnuk.cmd_select_openpgp()
    if fileid == 0:
        data_in_device = gnuk.cmd_get_data(0x00, 0x4f)
        for d in data_in_device:
            print "%02x" % d,
        print
        compare(data, data_in_device[8:])
    elif fileid >= 1 and fileid <= 4:
        data_in_device = gnuk.cmd_read_binary(fileid)
        compare(data, data_in_device)
    elif fileid == 5:
        data_in_device = gnuk.cmd_get_data(0x7f, 0x21)
        compare(data, data_in_device)

    gnuk.connection.disconnect()
    return 0


if __name__ == '__main__':
    passwd = DEFAULT_PW3
    if sys.argv[1] == '-p':
        from getpass import getpass
        passwd = getpass("Admin password: ")
        sys.argv.pop(1)
    if sys.argv[1] == '-u':
        is_update = True
        sys.argv.pop(1)
    else:
        is_update = False
    if sys.argv[1] == '-s':
        fileid = 0              # serial number
        filename = sys.argv[2]
        f = open(filename)
        email = os.environ['EMAIL']
        serial_data_hex = None
        for line in f.readlines():
            field = string.split(line)
            if field[0] == email:
                serial_data_hex = field[1].replace(':','')
        f.close()
        if not serial_data_hex:
            print "No serial number"
            exit(1)
        print "Writing serial number"
        data = binascii.unhexlify(serial_data_hex)
    elif sys.argv[1] == '-k':   # firmware update key
        keyno = sys.argv[2]
        fileid = 1 + int(keyno)
        filename = sys.argv[3]
        f = open(filename)
        data = f.read()
        f.close()
    else:
        fileid = 5              # Card holder certificate
        filename = sys.argv[1]
        f = open(filename)
        data = f.read()
        f.close()
        print "%s: %d" % (filename, len(data))
        print "Updating card holder certificate"
    main(fileid, is_update, data, passwd)
