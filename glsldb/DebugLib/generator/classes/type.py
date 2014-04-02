

class TypeOutputGenerator(CustomOutputGenerator):
    #
    # Type generation
    def genType(self, typeinfo, name):
        super(TypeOutputGenerator, self).genType(typeinfo, name)
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


class TypeGeneratorOptions(CustomGeneratorOptions):
    generatorClass = TypeOutputGenerator
