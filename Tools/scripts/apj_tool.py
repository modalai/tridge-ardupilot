#!/usr/bin/env python
'''
tool to manupulate ArduPilot firmware files, changing default parameters
'''

import os, sys, struct, json, base64, zlib, hashlib

import argparse

class embedded_defaults(object):
    '''class to manipulate embedded defaults in a firmware'''
    def __init__(self, filename):
        self.filename = filename
        self.offset = 0
        self.extension = os.path.splitext(filename)[1]
        if self.extension.lower() in ['.apj', '.px4']:
            self.load_apj()
        elif self.extension.lower() in ['.abin']:
            self.load_abin()
        else:
            self.load_binary()

    def load_binary(self):
        '''load firmware from binary file'''
        f = open(self.filename,'r')
        self.firmware = f.read()
        f.close()
        print("Loaded binary file of length %u" % len(self.firmware))

    def load_abin(self):
        '''load firmware from abin file'''
        f = open(self.filename,'r')
        self.headers = []
        while True:
            line = f.readline().rstrip()
            if line == '--':
                break
            self.headers.append(line)
            if len(self.headers) > 50:
                print("Error: too many abin headers")
                sys.exit(1)
        self.firmware = f.read()
        f.close()
        print("Loaded abin file of length %u" % len(self.firmware))

    def load_apj(self):
        '''load firmware from a json apj or px4 file'''
        f = open(self.filename,'r')
        self.fw_json = json.load(f)
        f.close()
        image = self.fw_json['image']
        self.firmware = zlib.decompress(base64.b64decode(self.fw_json['image']))
        print("Loaded apj file of length %u" % len(self.firmware))

    def save_binary(self):
        '''save binary file'''
        f = open(self.filename, 'w')
        f.write(self.firmware)
        f.close()
        print("Saved binary of length %u" % len(self.firmware))

    def save_apj(self):
        '''save apj file'''
        self.fw_json['image'] = base64.b64encode(zlib.compress(self.firmware, 9))
        f = open(self.filename,'w')
        json.dump(self.fw_json,f,indent=4)
        f.truncate()
        f.close()
        print("Saved apj of length %u" % len(self.firmware))

    def save_abin(self):
        '''save abin file'''
        f = open(self.filename,'w')
        for i in range(len(self.headers)):
            line = self.headers[i]
            if line.startswith('MD5: '):
                h = hashlib.new('md5')
                h.update(self.firmware)
                f.write('MD5: %s\n' % h.hexdigest())
            else:
                f.write(line+'\n')
        f.write('--\n')
        f.write(self.firmware)
        f.close()
        print("Saved abin of length %u" % len(self.firmware))

    def find(self):
        '''find defaults in firmware'''
        # these are the magic headers from AP_Param.cpp
        magic_str = "PARMDEF"
        param_magic = [ 0x55, 0x37, 0xf4, 0xa0, 0x38, 0x5d, 0x48, 0x5b ]
        while True:
            i = self.firmware[self.offset:].find(magic_str)
            if i == -1:
                return None
            matched = True
            for j in range(len(param_magic)):
                if ord(self.firmware[self.offset+i+j+8]) != param_magic[j]:
                    matched = False
                    break
            if not matched:
                self.offset += i+8
                continue
            self.offset += i
            self.max_len, self.length = struct.unpack("<HH", self.firmware[self.offset+16:self.offset+20])
            return True
    
    def contents(self):
        '''return current contents'''
        return self.firmware[self.offset+20:self.offset+20+self.length]

    def set_file(self, filename):
        '''set defaults to contents of a file'''
        print("Setting defaults from %s" % filename)
        f = open(filename, 'r')
        contents = f.read()
        f.close()
        length = len(contents)
        if length > self.max_len:
            print("Error: Length %u larger than maximum %u" % (length, self.max_len))
            sys.exit(1)
        new_fw = self.firmware[:self.offset+18]
        new_fw += struct.pack("<H", length)
        new_fw += contents
        new_fw += self.firmware[self.offset+20+length:]
        self.firmware = new_fw

    def save(self):
        '''save new firmware'''
        if self.extension.lower() in ['.apj', '.px4']:
            self.save_apj()
        elif self.extension.lower() in ['.abin']:
            self.save_abin()
        else:
            self.save_binary()

def defaults_contents(firmware, ofs, length):
    '''return current defaults contents'''
    return firmware

parser = argparse.ArgumentParser()

parser.add_argument('input_file')
parser.add_argument('--set-file', type=str, default=None)
parser.add_argument('--show', action='store_true', default=False)

args = parser.parse_args()

defaults = embedded_defaults(args.input_file)

if not defaults.find():
    print("Error: Param defaults support not found in firmware")
    sys.exit(1)
    
print("Found param defaults max_length=%u length=%u" % (defaults.max_len, defaults.length))

if args.show:
    print(defaults.contents())

if args.set_file:
    defaults.set_file(args.set_file)
    defaults.save()
