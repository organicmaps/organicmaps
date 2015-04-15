import os
import sys
import hashlib

lowPDefine = "LOW_P"
lowPSearch = "lowp"
mediumPDefine = "MEDIUM_P"
mediumPSearch = "mediump"
highPDefine = "HIGH_P"
highPSearch = "highp"
maxPrecDefine = "MAXPREC_P"
maxPrecSearch = "MAXPREC"

def formatOutFilePath(baseDir, fileName):
    return os.path.join(baseDir, "..", fileName)

def formatShaderSourceName(shaderFileName):
    shaderSourceName = shaderFileName
    return shaderSourceName.replace(".", "_").upper()

def formatShaderIndexName(shaderFileName):
    return formatShaderSourceName(shaderFileName) + "_INDEX"

def formatShaderDocName(shaderName):
    return shaderName.replace(".", "_") + ".txt"

def readIndexFile(filePath):
    f = open(filePath)
    gpuPrograms = dict()
    for line in f:
        lineParts = line.strip().split()
        if len(lineParts) != 3:
            print("Incorrect GPU program definition : " + line)
            exit(10)

        vertexShader   = next( f for f in lineParts if f.endswith(".vsh"))
        fragmentShader = next( f for f in lineParts if f.endswith(".fsh"))

        if not vertexShader:
            print("Vertex shader not found in GPU program definition : " + line)
            exit(11)

        if not fragmentShader:
            print("Fragment shader not found in GPU program definition : " + line)
            exit(12)

        if lineParts[0] in gpuPrograms.keys():
            print("More than one difinition of %s gpu program" % lineParts[0])
            exit(13)

        gpuPrograms[lineParts[0]] = (vertexShader, fragmentShader)

    return gpuPrograms

def generateShaderIndexes(shaders):
    return dict((v, k) for k, v in enumerate(shaders))

def generateProgramIndex(programs):
    return dict((v, k) for k, v in enumerate(programs))

def definitionChanged(newHeaderContent, defFilePath):
    if not os.path.isfile(defFilePath):
        return True

    defContent = open(defFilePath, 'r').read()
    oldMD5 = hashlib.md5()
    oldMD5.update(defContent)

    newMd5 = hashlib.md5()
    newMd5.update(newHeaderContent)

    return oldMD5.digest() != newMd5.digest()

def writeDefinitionFile(programIndex):
    result = ""
    result += "#pragma once\n\n"
    result += "#include \"std/map.hpp\"\n"
    result += "#include \"std/string.hpp\"\n"
    result += "#include \"std/target_os.hpp\"\n"
    result += "#include \"std/vector.hpp\"\n\n"
    result += "namespace gpu\n"
    result += "{\n"
    result += "\n"
    result += "#if defined(OMIM_OS_DESKTOP) && !defined(COMPILER_TESTS)\n"
    result += "  #define %s \"\" \n" % (lowPDefine)
    result += "  #define %s \"\" \n" % (mediumPDefine)
    result += "  #define %s \"\" \n" % (highPDefine)
    result += "  #define %s \"\" \n" % (maxPrecDefine)
    result += "#else\n"
    result += "  #define %s \"%s\"\n" % (lowPDefine, lowPSearch)
    result += "  #define %s \"%s\"\n" % (mediumPDefine, mediumPSearch)
    result += "  #define %s \"%s\"\n" % (highPDefine, highPSearch)
    result += "  #define %s \"%s\"\n" % (maxPrecDefine, maxPrecSearch)
    result += "#endif\n\n"
    result += "struct ProgramInfo\n"
    result += "{\n"
    result += "  ProgramInfo();\n"
    result += "  ProgramInfo(int vertexIndex, int fragmentIndex,\n"
    result += "              char const * vertexSource, char const  * fragmentSource);\n"
    result += "  int m_vertexIndex;\n"
    result += "  int m_fragmentIndex;\n"
    result += "  char const * m_vertexSource;\n"
    result += "  char const * m_fragmentSource;\n"
    result += "};\n\n"

    for programName in programIndex.keys():
        result += "extern int const %s;\n" % (programName)

    result += "\n"
    result += "void InitGpuProgramsLib(map<int, ProgramInfo> & gpuIndex);\n\n"
    result += "#if defined(COMPILER_TESTS)\n"
    result += "extern vector<string> VertexEnum;\n"
    result += "extern vector<string> FragmentEnum;\n\n"
    result += "void InitEnumeration();\n\n"
    result += "#endif\n"
    result += "} // namespace gpu\n"

    return result

def writeShader(outputFile, shaderFile, shaderDir):
    outputFile.write("  static char const %s[] = \" \\\n" % (formatShaderSourceName(shaderFile)))
    outputFile.write("  #ifdef GL_ES \\n\\\n")
    outputFile.write("    #ifdef GL_FRAGMENT_PRECISION_HIGH \\n\\\n")
    outputFile.write("      #define MAXPREC \" HIGH_P \" \\n\\\n")
    outputFile.write("    #else \\n\\\n")
    outputFile.write("      #define MAXPREC \" MEDIUM_P \" \\n\\\n")
    outputFile.write("    #endif \\n\\\n")
    outputFile.write("    precision MAXPREC float; \\n\\\n")
    outputFile.write("  #endif \\n\\\n")

    for line in open(os.path.join(shaderDir, shaderFile)):
        if not line.lstrip().startswith("//"):
            outputLine = line.rstrip().replace(lowPSearch, "\" " + lowPDefine + " \"")
            outputLine = outputLine.replace(mediumPSearch, "\" " + mediumPDefine + " \"")
            outputLine = outputLine.replace(highPSearch, "\" " + highPDefine+ " \"")
            outputFile.write("  %s \\n\\\n" % (outputLine))
    outputFile.write("  \";\n\n")

def writeShadersIndex(outputFile, shaderIndex):
    for shader in shaderIndex:
        outputFile.write("#define %s %s\n" % (formatShaderIndexName(shader), shaderIndex[shader]))

def writeImplementationFile(programsDef, programIndex, shaderIndex, shaderDir, implFile, defFile, shaders):
    vertexShaders = [s for s in shaders if s.endswith(".vsh")]
    fragmentShaders = [s for s in shaders if s.endswith(".fsh")]
    file = open(formatOutFilePath(shaderDir, implFile), 'w')
    file.write("#include \"%s\"\n\n" % (defFile))
    file.write("#include \"std/utility.hpp\"\n\n")

    file.write("namespace gpu\n")
    file.write("{\n\n")

    for shader in shaderIndex.keys():
        writeShader(file, shader, shaderDir)

    file.write("//---------------------------------------------//\n")
    writeShadersIndex(file, shaderIndex)
    file.write("//---------------------------------------------//\n")
    file.write("#if defined(COMPILER_TESTS)\n")
    file.write("vector<string> VertexEnum;\n")
    file.write("vector<string> FragmentEnum;\n\n")
    file.write("#endif\n")
    file.write("//---------------------------------------------//\n")
    for program in programIndex:
        file.write("const int %s = %s;\n" % (program, programIndex[program]));
    file.write("//---------------------------------------------//\n")
    file.write("\n")
    file.write("ProgramInfo::ProgramInfo()\n")
    file.write("  : m_vertexIndex(-1)\n")
    file.write("  , m_fragmentIndex(-1)\n")
    file.write("  , m_vertexSource(NULL)\n")
    file.write("  , m_fragmentSource(NULL) {}\n\n")
    file.write("ProgramInfo::ProgramInfo(int vertexIndex, int fragmentIndex,\n")
    file.write("                         char const * vertexSource, char const * fragmentSource)\n")
    file.write("  : m_vertexIndex(vertexIndex)\n")
    file.write("  , m_fragmentIndex(fragmentIndex)\n")
    file.write("  , m_vertexSource(vertexSource)\n")
    file.write("  , m_fragmentSource(fragmentSource)\n")
    file.write("{\n")
    file.write("}\n")
    file.write("\n")
    file.write("void InitGpuProgramsLib(map<int, ProgramInfo> & gpuIndex)\n")
    file.write("{\n")
    for program in programsDef.keys():
        vertexShader = programsDef[program][0]
        vertexIndexName = formatShaderIndexName(vertexShader)
        vertexSourceName = formatShaderSourceName(vertexShader)

        fragmentShader = programsDef[program][1]
        fragmentIndexName = formatShaderIndexName(fragmentShader)
        fragmentSourceName = formatShaderSourceName(fragmentShader)

        file.write("  gpuIndex.insert(make_pair(%s, ProgramInfo(%s, %s, %s, %s)));\n" % (program, vertexIndexName, fragmentIndexName, vertexSourceName, fragmentSourceName))
    file.write("}\n\n")
    file.write("#if defined(COMPILER_TESTS)\n")
    file.write("void InitEnumeration()\n")
    file.write("{\n")
    
    for s in vertexShaders:
        file.write("  VertexEnum.push_back(string(%s));\n" % formatShaderSourceName(s))
    
    for s in fragmentShaders:
        file.write("  FragmentEnum.push_back(string(%s));\n" % formatShaderSourceName(s))
    
    file.write("}\n\n")
    file.write("#endif\n")
    file.write("} // namespace gpu\n")
    file.close()

def validateDocumentation(shaders, shaderDir):
    docFiles = [file for file in os.listdir(os.path.join(shaderDir, "doc"))]

    undocumentedShaders = []
    for shader in shaders:
        if formatShaderDocName(shader) not in docFiles:
            undocumentedShaders.append(shader)
    # TODO(AlexZ): Commented out lines below to avoid qtcreator console spamming.
    #if undocumentedShaders:
        #print("no documentation for shaders:", undocumentedShaders)
        #exit(20)

if len(sys.argv) < 4:
  print("Usage : " + sys.argv[0] + " <shader_dir> <index_file_name> <generate_file_name>")
  exit(1)

shaderDir = sys.argv[1]
indexFileName = sys.argv[2]
definesFile = sys.argv[3] + ".hpp"
implFile = sys.argv[3] + ".cpp"

shaders = [file for file in os.listdir(shaderDir) if os.path.isfile(os.path.join(shaderDir, file)) and (file.endswith(".vsh") or file.endswith(".fsh"))]
shaderIndex = generateShaderIndexes(shaders)

programDefinition = readIndexFile(os.path.join(shaderDir, indexFileName))
programIndex = generateProgramIndex(programDefinition)

headerFile = writeDefinitionFile(programIndex)
if definitionChanged(headerFile, formatOutFilePath(shaderDir, definesFile)):
    f = open(formatOutFilePath(shaderDir, definesFile), 'w')
    f.write(headerFile)
    f.close()
# TODO(AlexZ): Commented out lines below to avoid qtcreator console spamming.
#else:
    #print("No need to update definition file")
writeImplementationFile(programDefinition, programIndex, shaderIndex, shaderDir, implFile, definesFile, shaders)
validateDocumentation(shaders, shaderDir)
