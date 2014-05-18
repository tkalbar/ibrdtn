#!/usr/bin/python

import glob
import os
import sys
#os.chdir("/")

newFile = open(sys.argv[1], 'w+')
newFile.write("x y\n")
for file in glob.glob("*.dat"):
    #newFile.write("x y")
    fname = os.path.basename(file)
    #label = fname[len(fname)-5]
    fTokens = fname.strip().split('-')
    last = fTokens[len(fTokens)-1]
    fTokens = last.strip().split('.')
    label = fTokens[0]
    
    oldFile = open(file)
    list = []
    for line in oldFile:
    	tokens = line.strip().split()
    	del tokens[0]
    	#tokens.append(label)
    	list.append(tuple(tokens))
    #print list
    if list:
        #newFile = open("pgy_"+label+".txt", 'w+')
        #newFile.write("x y\n")
        newFile.write('\n'.join('%s %s' % x for x in list))
        newFile.write("\n")
        #newFile.close()
    oldFile.close()
newFile.close()