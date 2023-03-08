#!/usr/bin/python

from os import listdir, system, getcwd
from os.path import isfile, join, splitext
from shutil import copyfile
import applescript

pathToLineFiles = getcwd()
lineFilesExt = ".line"	

filesList = [f for f in listdir(pathToLineFiles) if isfile(join(pathToLineFiles, f))]
lineFilesList = [f for f in filesList if splitext(f)[1] == lineFilesExt]

if not lineFilesList:
	cmd = """osascript -e '
	tell application id "com.apple.Terminal"
		set T to do script
    	set W to the id of window 1 where its tab 1 = T
    	do script "cd '"""+ pathToLineFiles + """'" in T
		do script "line" in T
	end tell
	'"""
	system(cmd)	
else: 
	for f in lineFilesList:
		lineFileName = f[:-5]
		
		cmd = """osascript -e '
		tell application id "com.apple.Terminal"
	    	set T to do script
	    	set W to the id of window 1 where its tab 1 = T
	    	do script "cd '"""+ pathToLineFiles + """'" in T
	    	do script "line '""" + 	lineFileName + """'" in T
		end tell
		'"""
		system(cmd)
