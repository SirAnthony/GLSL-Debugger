
from reg import OutputGenerator, GeneratorOptions, regSortFeatures, write
from datetime import datetime
import sys

header = """/***********************************************************
 *
 *   THIS FILE IS GENERATED AUTOMATICALLY {date}
 *
 * Copyright (c) {year} Python generator
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the \"Software\"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 ***********************************************************/"""


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
                 genFuncPointers = False,
                 protectFeature = True):
        GeneratorOptions.__init__(self, filename, apiname, profile,
                                  versions, emitversions, defaultExtensions,
                                  addExtensions, removeExtensions, sortProcedure)
        self.protectFeature  = protectFeature
        self.genFuncPointers = genFuncPointers


class CustomOutputGenerator(OutputGenerator):
    """Generate specified API interfaces in a specific style, such as a C header"""

    # Begin and end tokens of feature. Subclasses can overload this
    featureBegin = ('#ifdef', '')
    featureEnd = ('#endif /*', '*/')

    def __init__(self,
                 errFile = sys.stderr,
                 warnFile = sys.stderr,
                 diagFile = sys.stdout):
        OutputGenerator.__init__(self, errFile, warnFile, diagFile)
        # Internal state - accumulators for different inner block text
        self.typeBody = ''
        self.enumBody = ''
        self.cmdBody = ''

        # Subclasses can specify some headers and footers for files
        self.header = ''
        self.footer = ''

    def newline(self):
        write('', file=self.outFile)

    def beginFile(self, genOpts):
        OutputGenerator.beginFile(self, genOpts)
        #
        # User-supplied prefix text, if any (list of strings)
        date = datetime.now()
        write(header.format(date=date.strftime("%d.%m.%y %H:%M:%S"),
                year=date.year), file=self.outFile)
        if self.header:
            write(self.header)

    def endFile(self):
        self.newline()
        if self.footer:
            write(self.footer)
        OutputGenerator.endFile(self)

    def beginFeature(self, interface, emit):
        OutputGenerator.beginFeature(self, interface, emit)
        self.typeBody = ''
        self.enumBody = ''
        self.cmdPointerBody = ''
        self.cmdBody = ''

    def endFeature(self):
        if (self.emit):
            self.newline()
            if (self.genOpts.protectFeature):
                write(self.featureBegin[0], self.featureName,
                        self.featureBegin[1], file=self.outFile)
            if (self.enumBody != ''):
                write(self.enumBody, end='', file=self.outFile)
            if (self.genOpts.genFuncPointers and self.cmdPointerBody != ''):
                write(self.cmdPointerBody, end='', file=self.outFile)
            if (self.genOpts.protectFeature):
                write(self.featureEnd[0], self.featureName,
                        self.featureEnd[1], file=self.outFile)
        OutputGenerator.endFeature(self)

    def genType(self, typeinfo, name):
        pass

    def genEnum(self, enuminfo, name):
        pass

    def genCmd(self, cmdinfo, name):
        pass
