#! /usr/bin/env python

"""
Creates a skeleton of an evaluator
"""

__version__ = "1.0"
__author__  = "Roger Pawlowski"
__date__    = "Sept 23, 2009"

import string

#############################################################################

def main():
    """Creates an evaluator skeleton given a name"""

    from optparse import OptionParser

    usage = "usage: %prog [options] <class_name> <filename>"

    parser = OptionParser(usage)

    parser.add_option("-c","--copyright",
                      dest="copyright", default=True,
                      help="include header keyword to later insert copyright")
    
    parser.add_option("-i", "--incl_guard", dest="include_guard",
                      help="name for include guard in header file [default=<uppercased class name>")

    parser.add_option("-n","--no_eti", action="store_true",
                      dest="no_eti", default=False,
                      help="disables code for explicit template instantiation [default=%default]")
    
    parser.add_option("-g","--eti_filename",
                      dest="eti_filename",
                      default="ExplicitTemplateInstantiation.hpp",
                      help="filename for explicit template instantiation")
    
    parser.add_option("-k", "--h_suffix", dest="header_suffix",
                      default="hpp",
                      help="suffix for header file [defalt=%default]")

    parser.add_option("-t", "--template_impl_suffix",
                      dest="template_impl_suffix",
                      default="_Def",
                      help="template implementaiton suffix for header file [defalt=%default]")

    parser.add_option("-s", "--src_suffix", dest="src_suffix",
                      default="cpp",
                      help="suffix for source file [defalt=%default]")

    parser.add_option("-p", "--plist_name", dest="plist_name",
                      default="p",
                      help="parameter list name in ctor [defalt=%default]")

    parser.add_option("-d", "--data_name", dest="data_name",
                      default="d",
                      help="data name in post reg. setup method [defalt=%default]")

    parser.add_option("-f", "--field_manager_name", dest="fm_name",
                      default="fm",
                      help="field manager name in post reg. setup method [defalt=%default]")

    parser.add_option("-w", "--eval_data_name", dest="eval_data_name",
                      default="workset",
                      help="evaluation data name in evaluate method [defalt=%default]")

    parser.add_option("-a", "--add_pre_post_methods", dest="add_pp_methods",
                      action="store_true", default=False,
                      help="enable the pre/post methods [defalt=%default]")

    parser.add_option("-x", "--pre_data_name", dest="pre_data_name",
                      default="d",
                      help="data name for pre evaluate method [defalt=%default]")
    
    parser.add_option("-y", "--post_data_name", dest="post_data_name",
                      default="d",
                      help="data name for post evaluate method [defalt=%default]")

    (options, args) = parser.parse_args()

    if len(args) != 2:
        parser.error("incorrect number of arguments, requires 2!")

    class_name = args[0]
    base_file_name = args[1]

    header_file_name = base_file_name + "." + options.header_suffix
    definition_file_name = base_file_name + options.template_impl_suffix \
                           + "." + options.header_suffix
    src_file_name = base_file_name + "." + options.src_suffix
    
    header_file = open(header_file_name, 'w')

    include_guard = string.upper(class_name) + "_" + \
                    string.upper(options.header_suffix)

    if options.include_guard:
        include_guard = options.include_guard

    # *******************
    # Write out header
    # *******************

    if options.copyright:
        header_file.write("// @HEADER\n")
        header_file.write("// @HEADER\n\n")
    
    header_file.write("#ifndef " + include_guard + "\n")
    header_file.write("#define " + include_guard + "\n\n")
    header_file.write("#include \"Phalanx_Evaluator_Macros.hpp\"\n")
    header_file.write("#include \"Phalanx_MDField.hpp\"\n\n")
    if options.add_pp_methods:
        header_file.write("PHX_EVALUATOR_CLASS_PP(" + class_name + ")\n\n\n\n")
    else:
        header_file.write("PHX_EVALUATOR_CLASS(" + class_name + ")\n\n\n\n")

    header_file.write("PHX_EVALUATOR_CLASS_END\n\n")
    if options.no_eti:
        header_file.write("#include \"" + definition_file_name + "\"\n\n")
    else:
        header_file.write("#ifndef PHX_ETI\n")
        header_file.write("#include \"" + definition_file_name + "\"\n")
        header_file.write("#endif\n\n")
        
    header_file.write("#endif\n\n")
    header_file.close()

    # *******************
    # Write out definition
    # *******************
    def_file = open(definition_file_name, 'w')

    if options.copyright:
        def_file.write("// @HEADER\n")
        def_file.write("// @HEADER\n\n")
    
    def_file.write("#include \"Teuchos_TestForException.hpp\"")
    def_file.write("#include \"Phalanx_DataLayout.hpp\"\n")
    def_file.write("\n//****************************************************" \
                   + "******************\n")
    def_file.write("PHX_EVALUATOR_CTOR(" + class_name + "," +
                   options.plist_name + ")\n{\n\n}\n")
    
    def_file.write("\n//****************************************************" \
                   + "******************\n")
    def_file.write("PHX_POST_REGISTRATION_SETUP(" + class_name + "," +
                   options.plist_name + "," + options.fm_name + ")\n{\n\n}\n")
    def_file.write("\n//****************************************************" \
                   + "******************\n")
    def_file.write("PHX_EVALUATE_FIELDS(" + class_name + "," +
                   options.eval_data_name + ")\n{\n\n}\n")
    def_file.write("\n//****************************************************" \
                   + "******************\n")
    if options.add_pp_methods:
        def_file.write("PHX_PRE_EVALUATE_FIELDS(" + class_name + "," +
                       options.pre_data_name + ")\n{\n\n}\n")
        def_file.write("\n//****************************************************" \
                       + "******************\n")
        def_file.write("PHX_POST_EVALUATE_FIELDS(" + class_name + "," +
                       options.post_data_name + ")\n{\n\n}\n")
        def_file.write("\n//****************************************************" \
                       + "******************\n")
        

    
    def_file.close()

    # *******************
    # Write out explicit instantiation file
    # *******************
    if not options.no_eti:
        src_file = open(src_file_name, 'w')
        
        if options.copyright:
            src_file.write("// @HEADER\n")
            src_file.write("// @HEADER\n\n")
            
        src_file.write("#include \"" + options.eti_filename + "\"\n\n")
        src_file.write("#include \"" + header_file_name + "\"\n")
        src_file.write("#include \"" + definition_file_name + "\"\n\n")
        src_file.write("PHX_INSTANTIATE_TEMPLATE_CLASS(" + class_name + \
                       ")\n\n")
        src_file.write("#endif\n\n")
        src_file.close()

#############################################################################
# If called from the command line, call main()
#############################################################################

if __name__ == "__main__":
    main()
