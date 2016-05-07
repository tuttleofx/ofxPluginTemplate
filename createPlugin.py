#!/usr/bin/env python

import sys
import os
import json


def replaceString(inputStr, params):
    '''
    '''
    outputStr = str(inputStr)
    for key, value in params.iteritems():
        outputStr = outputStr.replace(key.upper(), value)
    return outputStr


def replaceInFile(filepath, params):
    '''
    '''
    with open(filepath, 'r') as dataFileIn:
        inputStr = dataFileIn.read()
    outputStr = replaceString(inputStr, params)
    if inputStr != outputStr:
        with open(filepath, 'w') as dataFileOut:
            dataFileOut.write(outputStr)


def renameFile(filepath, params):
    '''
    '''
    folder, filename = os.path.split(filepath)
    newFilename = replaceString(filename, params)
    newFilepath = os.path.join(folder, newFilename)
    if filename != newFilename:
        os.rename(filepath, newFilepath)
    return newFilepath


if  __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Rename the template files to create a new OpenFX Plugin.')
    parser.add_argument('params', help='JSON files with parameters')
    parser.add_argument('path', help='Path to the OFX template')
    args = parser.parse_args()

    with open(args.params, 'r') as paramsFile:
        params = json.load(paramsFile)

    # print "args.params:", args.params
    # print "args.path:", args.path
    for root, folders, filenames in os.walk(args.path):
        # do not enter into hidden folders and openfx folder
        folders[:] = [folder for folder in folders if folder[0] != '.' and folder != 'openfx']
        for filename in filenames:
            filepath = os.path.join(root, filename)
            newFilepath = renameFile(filepath, params)
            replaceInFile(newFilepath, params)
