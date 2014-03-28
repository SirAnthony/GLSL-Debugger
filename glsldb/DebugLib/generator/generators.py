#!/usr/bin/env python
################################################################################
#
# Copyright (c) 2014 SirAnthony <anthony at adsorbtion.org>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
################################################################################

from reg import OutputGenerator, GeneratorOptions, regSortFeatures, write
import sys

class CustomOutputGenerator(OutputGenerator):
    """Generate specified API interfaces in a specific style, such as a C header"""
    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)
        # Internal state - accumulators for different inner block text
        self.typeBody = ''
        self.enumBody = ''
        self.cmdBody = ''
     #
    def newline(self):
        write('', file=self.outFile)
    #
    def beginFile(self, genOpts):
        OutputGenerator.beginFile(self, genOpts)
        #
        # User-supplied prefix text, if any (list of strings)
        if (genOpts.prefixText):
            for s in genOpts.prefixText:
                write(s, file=self.outFile)
        #
        # Some boilerplate describing what was generated - this
        # will probably be removed later since the extensions
        # pattern may be very long.
        write('/* Generated header for:', file=self.outFile)
        write(' * API:', genOpts.apiname, file=self.outFile)
        if (genOpts.profile):
            write(' * Profile:', genOpts.profile, file=self.outFile)
        write(' * Versions considered:', genOpts.versions, file=self.outFile)
        write(' * Versions emitted:', genOpts.emitversions, file=self.outFile)
        write(' * Default extensions included:', genOpts.defaultExtensions, file=self.outFile)
        write(' * Additional extensions included:', genOpts.addExtensions, file=self.outFile)
        write(' * Extensions removed:', genOpts.removeExtensions, file=self.outFile)
        write(' */', file=self.outFile)
    def endFile(self):
        self.newline()
        # Finish processing in superclass
        OutputGenerator.endFile(self)
    def beginFeature(self, interface, emit):
        # Start processing in superclass
        OutputGenerator.beginFeature(self, interface, emit)
        # C-specific
        # Accumulate types, enums, function pointer typedefs, end function
        # prototypes separately for this feature. They're only printed in
        # endFeature().
        self.typeBody = ''
        self.enumBody = ''
        self.cmdPointerBody = ''
        self.cmdBody = ''
    def endFeature(self):
        # C-specific
        # Actually write the interface to the output file.
        if (self.emit):
            self.newline()
            if (self.genOpts.protectFeature):
                write('#ifdef', self.featureName, file=self.outFile)
            if (self.typeBody != ''):
                write(self.typeBody, end='', file=self.outFile)
            #
            # Don't add additional protection for derived type declarations,
            # which may be needed by other features later on.
            if (self.featureExtraProtect != None):
                write('#ifdef', self.featureExtraProtect, file=self.outFile)
            if (self.enumBody != ''):
                write(self.enumBody, end='', file=self.outFile)
            if (self.genOpts.genFuncPointers and self.cmdPointerBody != ''):
                write(self.cmdPointerBody, end='', file=self.outFile)
            if (self.featureExtraProtect != None):
                write('#endif /*', self.featureExtraProtect, '*/', file=self.outFile)
            if (self.genOpts.protectFeature):
                write('#endif /*', self.featureName, '*/', file=self.outFile)
        # Finish processing in superclass
        OutputGenerator.endFeature(self)

    def genType(self, typeinfo, name):
        pass

    def genEnum(self, enuminfo, name):
        pass

    def genCmd(self, cmdinfo, name):
        pass

class TypeOutputGenerator(CustomOutputGenerator):
    #
    # Type generation
    def genType(self, typeinfo, name):
        OutputGenerator.genType(self, typeinfo, name)
        #
        # Replace <apientry /> tags with an APIENTRY-style string
        # (from self.genOpts). Copy other text through unchanged.
        # If the resulting text is an empty string, don't emit it.
        typeElem = typeinfo.elem
        s = noneStr(typeElem.text)
        for elem in typeElem:
            if (elem.tag == 'apientry'):
                s += self.genOpts.apientry + noneStr(elem.tail)
            else:
                s += noneStr(elem.text) + noneStr(elem.tail)
        if (len(s) > 0):
            self.typeBody += s + "\n"

class EnumOutputGenerator(CustomOutputGenerator):
    #
    # Enumerant generation
    def genEnum(self, enuminfo, name):
        OutputGenerator.genEnum(self, enuminfo, name)
        # EnumInfo.type is a C value suffix (e.g. u, ull)
        self.enumBody += '#define ' + name.ljust(33) + ' ' + enuminfo.elem.get('value')
        # Handle non-integer 'type' fields by using it as the C value suffix
        t = enuminfo.elem.get('type')
        if (t != '' and t != 'i'):
            self.enumBody += enuminfo.type
        self.enumBody += "\n"

class CustomGeneratorOptions(GeneratorOptions):
    """Represents options during C header production from an API registry"""
    def __init__(self,
                 filename = None,
                 apiname = None,
                 profile = None,
                 versions = '.*',
                 emitversions = '.*',
                 defaultExtensions = None,
                 addExtensions = None,
                 removeExtensions = None,
                 sortProcedure = regSortFeatures,
                 prefixText = "",
                 genFuncPointers = False,
                 protectFeature = True):
        GeneratorOptions.__init__(self, filename, apiname, profile,
                                  versions, emitversions, defaultExtensions,
                                  addExtensions, removeExtensions, sortProcedure)
        self.prefixText      = prefixText
        self.protectFeature  = protectFeature
        self.genFuncPointers = genFuncPointers

class TypeGeneratorOptions(CustomGeneratorOptions):
    generatorClass = TypeOutputGenerator

class EnumGeneratorOptions(CustomGeneratorOptions):
    generatorClass = EnumOutputGenerator


