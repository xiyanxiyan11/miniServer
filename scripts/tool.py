#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import os, sys
import re
import filecmp

class MisakaTool(object):
    """manager tool for debug"""
    def matchfile(self, reg, dstfile):
        p = re.compile(reg)
        for mfile in os.listdir('./'):
                if mfile == dstfile:
                    continue
                if NONE == p.match(mfile):
                    continue
                if not filecmp.cmp(mfile, dstfile):
                    print("file %s missmatch " % mfile)
