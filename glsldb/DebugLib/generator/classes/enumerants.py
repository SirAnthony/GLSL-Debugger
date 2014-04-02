

from classes.custom import CustomOutputGenerator, CustomGeneratorOptions


enums_header = """

#define ADD_BITFIELD(name, value) hash_insert_many(bitfields, value, name);
#define ADD_ENUM(name, value) hash_insert(enumerants, value, name);

int load_enumerants(Hash* enumerants, Hash* bitfields)
{
\thash_create(enumerants, hashInt, compInt, 8192, 0);
\thash_create(bitfields, hashInt, compInt, 512, 0);

"""

enums_footer = """
}
"""

class EnumOutputGenerator(CustomOutputGenerator):

    header = enums_header
    footer = enums_footer

    #
    # Enumerant generation
    def genEnum(self, enuminfo, name):
        CustomOutputGenerator.genEnum(self, enuminfo, name)
        # Skip non-integer 'type'
        t = enuminfo.elem.get('type')
        if (t and t != 'i'):
            return

        group = enuminfo.elem.getparent()
        if group.get('type') == 'bitmask':
            self.enumBody += '\tADD_BITFIELD({0}, {1})'.format(
                name, enuminfo.elem.get('value'))
        else:
            self.enumBody += '\tADD_ENUMERANT({0}, {1})'.format(
                name, enuminfo.elem.get('value'))
        self.enumBody += "\n"


class EnumGeneratorOptions(CustomGeneratorOptions):
    generatorClass = EnumOutputGenerator
