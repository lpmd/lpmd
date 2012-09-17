#!/usr/bin/env python

import sys, string
from sys import *

def crdc(x):
    if x.startswith('<') and x.endswith('>'): return tuple(crdc(z) for z in x[1:-1].split(','))
    if not all((z in string.digits+'.+-eE') for z in x): return x
    try: return eval(x)
    except: return x

def cfmt(x):
    if type(x) == tuple: return '<'+','.join(cfmt(z) for z in x)+'>'
    return str(x)

def ProtectBrackets(txt, openbrac, closebrac):
    if not openbrac in txt and not closebrac in txt: return (txt, {})
    cnt, d, p0, newtxt = 0, {}, 0, ''
    while openbrac in txt[p0:]:
        p1 = txt.find(openbrac, p0)
        newtxt += txt[p0:p1]
        p2 = txt.find(closebrac, p1) 
        newtxt += '%'+str(cnt)
        d['%'+str(cnt)] = openbrac+txt[p1+1:p2]+closebrac
        p0, cnt = (p2+1), cnt+1
    return (newtxt, d)

def Substitute(x, d): return (x if x not in d else d[x])

def SplitLine(line): 
    line, d = ProtectBrackets(line, '<', '>')
    return [crdc(Substitute(x, d)) for x in line.split()]

class WrongHeader(Exception):
    def __init__(self, txt): self.txt = txt
    def __str__(self): return '[Error] Wrong header, \"%s\"' % self.txt 

class UnexpectedEndOfFile(Exception):
    def __init__(self, name): self.name = name
    def __str__(self): return '[Error] Unexpected end of file, \"%s\"' % self.name 

class Configuration(list):

    def __init__(self, tags): self.tags, self.cell = tags, [None, None, None]

    def Tag(self, tag, index): return (self[index][self.tags.index(tag)] if tag in self.tags else None) 

    def PackTags(self, tags, index): return tuple(self.Tag(t, index) for t in tags)

class LPMD2:

    def __init__(self, path=None):
        self.valid, self.tags, self.configs = False, [], []
        if path != None: self.Read(path)

    def __len__(self): return len(self.configs)

    def __getitem__(self, i): return self.configs[i]

    def IsValid(self): return self.valid

    def Read(self, path):
        self.valid = False
        f = file(path, 'r')
        header1 = f.readline().strip()
        if header1 != 'LPMD 2.0 L': raise WrongHeader(header1)
        header2 = f.readline().strip()
        if header2 == '': raise WrongHeader(header2)
        hspl = [x.strip() for x in header2.split()]
        if hspl[0] != 'HDR': raise WrongHeader(header2)
        self.tags = hspl[1:]
        while True:
              atoms = f.readline()
              if atoms == '': break
              this_config = Configuration(self.tags)
              cellvectors = [float(x.strip()) for x in f.readline().split()]
              for q in range(3): this_config.cell[q] = tuple(cellvectors[3*q:3*q+3])
              for i in xrange(int(atoms)):
                  line = f.readline()
                  if line == '': raise UnexpectedEndOfFile(path)
                  this_config.append(SplitLine(line))
              self.configs.append(this_config)
        self.valid = (len(self.configs) > 0)

    def ReadInPlace(self, path, temp_config, callback):
        self.valid = False
        f = file(path, 'r')
        header1 = f.readline().strip()
        if header1 != 'LPMD 2.0 L': raise WrongHeader(header1)
        header2 = f.readline().strip()
        if header2 == '':raise WrongHeader(header2)
        hspl = [x.strip() for x in header2.split()]
        if hspl[0] != 'HDR': raise WrongHeader(header2)
        self.tags = hspl[1:]
        while True:
              atoms = f.readline()
              if atoms == '': break
              this_config = Configuration(self.tags)
              cellvectors = [float(x.strip()) for x in f.readline().split()]
              for q in range(3): this_config.cell[q] = tuple(cellvectors[3*q:3*q+3])
              for i in xrange(int(atoms)):
                  line = f.readline()
                  if line == '': raise UnexpectedEndOfFile(path)
                  this_config.append(SplitLine(line))
              temp_config[0] = this_config
              callback(this_config)
        self.Valid = True

    def Write(self, path):
        f = file(path, 'w')
        f.write('LPMD 2.0 L\nHDR '+' '.join(self.tags)+'\n')
        for conf in self.configs:
            f.write('%d\n' % len(conf)) 
            cellvectors = list(conf.cell[0])+list(conf.cell[1])+list(conf.cell[2])
            f.write(' '.join(str(x) for x in cellvectors)+'\n')
            for i in range(len(conf)):
                for t in self.tags: f.write(' '+cfmt(conf.Tag(t, i)))
                f.write('\n')

