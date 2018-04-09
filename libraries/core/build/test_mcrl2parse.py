#!/usr/bin/env python

#~ Copyright 2011 Wieger Wesselink.
#~ Distributed under the Boost Software License, Version 1.0.
#~ (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#from difflib import *
from path import *
from mcrl2_tools import *
import time

# returns True, False or None if a timeout occurs
def run_mcrl2parse(filename, options = [], timeout = 10):
    dummy, text = timeout_command('mcrl2parse', options + ' ' + filename, timeout)
    return text

def main():
    t0 = time.time()
    for file in path('/home/wieger/svn/mcrl2/examples').walkfiles('*.mcrl2'):
        print file, run_mcrl2parse(file, '-P', 5)
    print time.time() - t0, "seconds wall time"

if __name__ == '__main__':
    main()
