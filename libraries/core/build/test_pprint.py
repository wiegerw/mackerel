#!/usr/bin/env python

#~ Copyright 2011 Wieger Wesselink.
#~ Distributed under the Boost Software License, Version 1.0.
#~ (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#from difflib import *
from path import *
from mcrl2_tools import *

# returns True, False or None if a timeout occurs
def run_pprint(filename, timeout = 10):
    command = 'pprint ' + filename
    text, dummy = timeout_command('pprint', '-r ' + filename, timeout)
    if text == None:
        print 'WARNING: timeout on "%s"' % command
        return None
    result = text
    if result.find('EQUAL!') != -1:
        return True
    else:
        diff, dummy = timeout_command('diff', '%s.1 %s.2' % (filename, filename), timeout)
        print '<diff>', diff
        return result + ' # ' + str(diff)
    print 'WARNING: unknown failure on "%s"' % command
    return None

def main():
    #for file in path('/home/wieger/svn/mcrl2/examples').walkfiles('*.mcrl2'):
    #    print file, run_pprint(file, 5)

    for file in path('/home/wieger/svn/mcrl2/examples').walkfiles('*.lps'):
        print file, run_pprint(file, 5)

if __name__ == '__main__':
    main()
