from lxml import etree
from XpetraDoxygenVersion import checkDoxygenVersion

#### GENERATE HEADER / FOOTER ####
def buildHeader( className, script='', prefix='' ):
    with open('../doc/COPYRIGHT_AND_LICENSE', 'r') as f:
         headerStr = f.read()
    upperClassName =  className.upper()
    headerStr += "#ifndef XPETRA_"   + prefix + upperClassName + "_HPP\n"
    headerStr += "#define XPETRA_"   + prefix + upperClassName + "_HPP\n\n"
    headerStr += "/* this file is automatically generated - do not edit (see script/" + script + ") */"
    return headerStr

def buildFooter( className, short=True):
    upperClassName =  className.upper()
    footerStr = ''
    if short: footerStr += "#define XPETRA_"   + upperClassName + "_SHORT\n"
    footerStr += "#endif // XPETRA_" + upperClassName + "_HPP"
    return footerStr

#### #### #### #### #### #### #### ####

#### PARSE INCLUDES ####
def buildInclude( XMLfile, skipList=[] ):
    tree = etree.parse(XMLfile)
    includes = tree.xpath('//includes/text()')
    includesStr = ''
    for include in includes:
        if include in skipList: continue
        if (include[0:6] == 'Tpetra'):
            includesStr += "#include \"" + include.replace('Tpetra','Xpetra') + "\"" + "\n"
        else:
            includesStr += "#include <" + include + ">" + "\n"
            
    includesStr = includesStr.rstrip()

    return includesStr
####

def buildDestructor( className ):
    return '    //! @name Constructor/Destructor Methods' + "\n" +  '    //@{ ' + "\n" +  '' + "\n" +  '    //! Destructor.' + "\n" +  '    virtual ~' + className + '() { }' + "\n" +  "\n" +  '   //@}'
        
#### PARSE CLASS DEFINITION ####

def buildClassDefinition( XMLfile, prefix='' ):
    tree = etree.parse(XMLfile)
    root = tree.getroot() # root == <doxygen>
    classNode = root[0]   # classNode == <compounddef>
    checkDoxygenVersion(root)
    
    className = classNode.xpath('compoundname')[0].text # Tpetra::Map
    className = className.lstrip('Tpetra::')            # Map
    #print(className)
    
    className = prefix + className

    return className
####

def buildTemplateParam( XMLfile ):
    tree = etree.parse(XMLfile)
    root = tree.getroot() # root == <doxygen>
    classNode = root[0]   # classNode == <compounddef>
    checkDoxygenVersion(root)
    
    templateParamNode = classNode.xpath('templateparamlist')[0];
    str = ''
    for child in templateParamNode:
        type   = child.xpath('type')[0].text        # == 'class'
        name   = child.xpath('declname')[0].text    # == 'GlobalOrdinal'
        
        defvalStr = ''
        defvalNode = child.xpath('defval')
        if len(defvalNode) == 1:
            defval = defvalNode[0].text             # == 'LocalOrdinal'
            defvalStr = ' = ' + defval

        str += type + ' ' + name + defvalStr + ', ' # str == 'class GlobalOrdinal = LocalOrdinal, '
        #print(str)
    
    templateParamStr = 'template <' + str.rstrip(', ') + '>' # template <class LocalOrdinal, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType>
    #print(templateParamStr) 

    return templateParamStr

def buildTemplateParam2( XMLfile ):
    tree = etree.parse(XMLfile)
    root = tree.getroot() # root == <doxygen>
    classNode = root[0]   # classNode == <compounddef>
    checkDoxygenVersion(root)
    
    templateParamNode = classNode.xpath('templateparamlist')[0];
    str = ''
    for child in templateParamNode:
        name   = child.xpath('declname')[0].text    # == 'GlobalOrdinal'
        str += name + ', ' # str == 'GlobalOrdinal, '
        #print(str)
    
    templateParamStr = str.rstrip(', ') # LocalOrdinal, GlobalOrdinal, Node
    #print(templateParamStr) 

    return templateParamStr
####

#### PARSE MEMBERDEF PUBLIC FUNCTIONS ####
def buildClassFunctions( XMLfile, skipFunctionList, buildFuncLine ):
    tree = etree.parse(XMLfile)
    root = tree.getroot() # root == <doxygen>
    classNode = root[0]   # classNode == <compounddef>
    checkDoxygenVersion(root)

    functionStr = ''

    sectiondefNodes = classNode.xpath("//sectiondef[@kind='user-defined']")
    for sectiondefNode in sectiondefNodes:
        header = sectiondefNode.xpath("header//text()")
        if len(header) > 0: header = header[0]
        else: header = ''
        #description = sectiondefNode.xpath("description")[0].xpath("string()")        

        ###publicFunctionNodes = classNode.xpath("//memberdef[@kind='function' and @prot='public']")
        publicFunctionNodes = sectiondefNode.xpath("memberdef[@kind='function' and @prot='public']")
        functionSubStr = ''
        for functionNode in publicFunctionNodes:
            # skip some functions
            name = functionNode.xpath('name')[0].text
            if name in skipFunctionList: continue

            # build the line for this function
            functionSubStr += buildFuncLine(functionNode)

        if functionSubStr != '':
            functionStr += "    //! @name " + header + "\n"
            functionStr += "    //@{\n\n"
            functionStr += functionSubStr
            functionStr += "    //@}\n\n"
            
    functionStr = functionStr.rstrip()
    
    return functionStr;

####
