import os
import sys

lowPDefine = "LOW_P"
lowPSearch = "lowp"
mediumPDefine = "MEDIUM_P"
mediumPSearch = "mediump"
highPDefine = "HIGH_P"
highPSearch = "highp"

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
            print "Incorrect GPU program definition : " + line
            exit(10)

        vertexShader   = next( f for f in lineParts if f.endswith(".vsh"))
        fragmentShader = next( f for f in lineParts if f.endswith(".fsh"))

        if not vertexShader:
            print "Vertex shader not found in GPU program definition : " + line
            exit(11)

        if not fragmentShader:
            print "Fragment shader not found in GPU program definition : " + line
            exit(12)

        if lineParts[0] in gpuPrograms.keys():
            print "More than one difinition of %s gpu program" % lineParts[0]
            exit(13)

        gpuPrograms[lineParts[0]] = (vertexShader, fragmentShader)

    return gpuPrograms

def generateShaderIndexes(shaders):
    return dict((v, k) for k, v in enumerate(shaders))

def generateProgramIndex(programs):
    return dict((v, k) for k, v in enumerate(programs))

def definitionChanged(programIndex, defFilePath):
    if not os.path.isfile(defFilePath):
        return True

    defContent = open(defFilePath, 'r').read()
    for program in programIndex.keys():
        if program not in defContent:
            return True

    return False

def writeDefinitionFile(programIndex, defFilePath):
    file = open(defFilePath, 'w')
    file.write("#pragma once\n\n")
    file.write("#include \"../std/map.hpp\"\n")
    file.write("#include \"../std/vector.hpp\"\n")
    file.write("#include \"../std/string.hpp\"\n\n")
    file.write("namespace gpu\n")
    file.write("{\n")
    file.write("\n")
    file.write("#if defined(OMIM_OS_DESKTOP) && !defined(COMPILER_TESTS)\n")
    file.write("  #define %s \"\" \n" % (lowPDefine))
    file.write("  #define %s \"\" \n" % (mediumPDefine))
    file.write("  #define %s \"\" \n" % (highPDefine))
    file.write("#else\n")
    file.write("  #define %s \"%s\"\n" % (lowPDefine, lowPSearch))
    file.write("  #define %s \"%s\"\n" % (mediumPDefine, mediumPSearch))
    file.write("  #define %s \"%s\"\n" % (highPDefine, highPSearch))
    file.write("#endif\n\n")
    file.write("struct ProgramInfo\n")
    file.write("{\n")
    file.write("  ProgramInfo();\n")
    file.write("  ProgramInfo(int vertexIndex, int fragmentIndex,\n")
    file.write("              char const * vertexSource, char const  * fragmentSource);\n")
    file.write("  int m_vertexIndex;\n")
    file.write("  int m_fragmentIndex;\n")
    file.write("  char const * m_vertexSource;\n")
    file.write("  char const * m_fragmentSource;\n")
    file.write("};\n\n")

    for programName in programIndex.keys():
        file.write("extern int const %s;\n" % (programName));

    file.write("\n")
    file.write("void InitGpuProgramsLib(map<int, ProgramInfo> & gpuIndex);\n\n")
    file.write("#if defined(COMPILER_TESTS)\n")
    file.write("extern vector<string> VertexEnum;\n")
    file.write("extern vector<string> FragmentEnum;\n\n")
    file.write("void InitEnumeration();\n\n")
    file.write("#endif\n")
    file.write("} // namespace gpu\n")
    file.close()

def writeShader(outputFile, shaderFile, shaderDir):
    outputFile.write("  static char const %s[] = \" \\\n" % (formatShaderSourceName(shaderFile)));
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
    file.write("#include \"../std/utility.hpp\"\n\n")

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
    if undocumentedShaders:
        print "no documentation for shaders:", undocumentedShaders
        #exit(20)

if len(sys.argv) < 4:
  print "Usage : " + sys.argv[0] + " <shader_dir> <index_file_name> <generate_file_name>"
  exit(1)

shaderDir = sys.argv[1]
indexFileName = sys.argv[2]
definesFile = sys.argv[3] + ".hpp"
implFile = sys.argv[3] + ".cpp"

shaders = [file for file in os.listdir(shaderDir) if os.path.isfile(os.path.join(shaderDir, file)) and (file.endswith(".vsh") or file.endswith(".fsh"))]
shaderIndex = generateShaderIndexes(shaders)

programDefinition = readIndexFile(os.path.join(shaderDir, indexFileName))
programIndex = generateProgramIndex(programDefinition)

if definitionChanged(programIndex, formatOutFilePath(shaderDir, definesFile)):
    writeDefinitionFile(programIndex, formatOutFilePath(shaderDir, definesFile))
else:
    print "No need to update definition file"
writeImplementationFile(programDefinition, programIndex, shaderIndex, shaderDir, implFile, definesFile, shaders)
validateDocumentation(shaders, shaderDir)
