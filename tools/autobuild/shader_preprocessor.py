import os
import sys
import hashlib

LOWP_DEFINE = "LOW_P"
LOWP_SEARCH = "lowp"
MEDIUMP_DEFINE = "MEDIUM_P"
MEDIUMP_SEARCH = "mediump"
HIGHP_DEFINE = "HIGH_P"
HIGHP_SEARCH = "highp"
MAX_PREC_DEFINE = "MAXPREC_P"
MAX_PREC_SEARCH = "MAXPREC"
SHADER_VERSION_DEFINE = "SHADER_VERSION"
VERTEX_SHADER_EXT = ".vsh.glsl"
FRAG_SHADER_EXT = ".fsh.glsl"
GLES3_PREFIX = "GLES3_"
GLES3_SHADER_PREFIX = "gles3_"

SHADERS_LIB_COMMON_PATTERN = "// Common"
SHADERS_LIB_VS_PATTERN = "// VS"
SHADERS_LIB_FS_PATTERN = "// FS"
SHADERS_LIB_COMMON_INDEX = 0
SHADERS_LIB_VS_INDEX = 1
SHADERS_LIB_FS_INDEX = 2


def format_shader_source_name(shader_file_name):
    shader_source_name = shader_file_name
    return shader_source_name.replace(".glsl", "").replace(".", "_").upper()


def format_shader_index_name(shader_file_name):
    return format_shader_source_name(shader_file_name) + "_INDEX"


def read_index_file(file_path):
    gpu_programs = dict()
    with open(file_path, 'r') as f:
        index = 0
        for line in f:
            line_parts = line.strip().split()
            if len(line_parts) != 3:
                print("Incorrect GPU program definition : " + line)
                exit(10)

            vertex_shader = next(f for f in line_parts if f.endswith(VERTEX_SHADER_EXT))
            fragment_shader = next(f for f in line_parts if f.endswith(FRAG_SHADER_EXT))

            if not vertex_shader:
                print("Vertex shader not found in GPU program definition : " + line)
                exit(11)

            if not fragment_shader:
                print("Fragment shader not found in GPU program definition : " + line)
                exit(12)

            if line_parts[0] in gpu_programs.keys():
                print("More than one difinition of %s gpu program" % line_parts[0])
                exit(13)

            gpu_programs[index] = (vertex_shader, fragment_shader, line_parts[0])
            index += 1

    return gpu_programs


def read_shaders_lib_file(file_path):
    shaders_library = ['', '', '']
    with open(file_path, 'r') as f:
        shaders_lib_content = f.read()
    if len(shaders_lib_content) == 0:
        return shaders_library

    common_index = shaders_lib_content.find(SHADERS_LIB_COMMON_PATTERN)
    if common_index < 0:
        print("Common functions block is not found in " + file_path)
        exit(14)
    vs_index = shaders_lib_content.find(SHADERS_LIB_VS_PATTERN)
    if vs_index < 0:
        print("Vertex shaders functions block is not found in " + file_path)
        exit(15)
    fs_index = shaders_lib_content.find(SHADERS_LIB_FS_PATTERN)
    if fs_index < 0:
        print("Vertex shaders functions block is not found in " + file_path)
        exit(16)
    if not (common_index < vs_index < fs_index):
        print("Order of functions block is incorrect in " + file_path)
        exit(17)

    shaders_library[SHADERS_LIB_COMMON_INDEX] = shaders_lib_content[common_index:vs_index - 1]
    shaders_library[SHADERS_LIB_VS_INDEX] = shaders_lib_content[vs_index:fs_index - 1]
    shaders_library[SHADERS_LIB_FS_INDEX] = shaders_lib_content[fs_index:]

    return shaders_library


def generate_shader_indexes(shaders):
    return dict((v, k) for k, v in enumerate(shaders))


def definition_changed(new_header_content, def_file_path):
    if not os.path.isfile(def_file_path):
        return True

    def_content = open(def_file_path, 'r').read()
    old_md5 = hashlib.md5()
    old_md5.update(def_content)

    new_md5 = hashlib.md5()
    new_md5.update(new_header_content)

    return old_md5.digest() != new_md5.digest()


def write_definition_file(program_index):
    result = ""
    result += "#pragma once\n\n"
    result += "#include \"drape/drape_global.hpp\"\n"
    result += "#include \"drape/gpu_program_info.hpp\"\n\n"
    result += "#include \"std/target_os.hpp\"\n\n"
    result += "#include <map>\n"
    result += "#include <string>\n"
    result += "#include <vector>\n\n"
    result += "namespace gpu\n"
    result += "{\n"
    result += "#if defined(OMIM_OS_DESKTOP) && !defined(COMPILER_TESTS)\n"
    result += "  #define %s \"\" \n" % (LOWP_DEFINE)
    result += "  #define %s \"\" \n" % (MEDIUMP_DEFINE)
    result += "  #define %s \"\" \n" % (HIGHP_DEFINE)
    result += "  #define %s \"\" \n" % (MAX_PREC_DEFINE)
    result += "  #define %s \"#version 150 core \\n\"\n" % (SHADER_VERSION_DEFINE)
    result += "#else\n"
    result += "  #define %s \"%s\"\n" % (LOWP_DEFINE, LOWP_SEARCH)
    result += "  #define %s \"%s\"\n" % (MEDIUMP_DEFINE, MEDIUMP_SEARCH)
    result += "  #define %s \"%s\"\n" % (HIGHP_DEFINE, HIGHP_SEARCH)
    result += "  #define %s \"%s\"\n" % (MAX_PREC_DEFINE, MAX_PREC_SEARCH)
    result += "  #define %s \"#version 300 es \\n\"\n" % (SHADER_VERSION_DEFINE)
    result += "#endif\n\n"

    for programName in program_index.keys():
        result += "extern int const %s;\n" % (program_index[programName][2])

    result += "\n"
    result += "#if !defined(COMPILER_TESTS)\n"
    result += "class ShaderMapper : public GpuProgramGetter\n"
    result += "{\n"
    result += "public:\n"
    result += "  ShaderMapper(dp::ApiVersion apiVersion);\n"
    result += "  GpuProgramInfo const & GetProgramInfo(int program) const override;\n"
    result += "private:\n"
    result += "  std::map<int, gpu::GpuProgramInfo> m_mapping;\n"
    result += "};\n"
    result += "#endif\n\n"
    result += "#if defined(COMPILER_TESTS)\n"
    result += "using ShadersEnum = std::vector<std::pair<std::string, std::string>>;\n"
    result += "extern ShadersEnum GetVertexShaders(dp::ApiVersion apiVersion);\n"
    result += "extern ShadersEnum GetFragmentShaders(dp::ApiVersion apiVersion);\n"
    result += "#endif\n"
    result += "} // namespace gpu\n"

    return result


def write_shader_gles_header(output_file):
    output_file.write("  #ifdef GL_ES \\n\\\n")
    output_file.write("    #ifdef GL_FRAGMENT_PRECISION_HIGH \\n\\\n")
    output_file.write("      #define MAXPREC \" HIGH_P \" \\n\\\n")
    output_file.write("    #else \\n\\\n")
    output_file.write("      #define MAXPREC \" MEDIUM_P \" \\n\\\n")
    output_file.write("    #endif \\n\\\n")
    output_file.write("    precision MAXPREC float; \\n\\\n")
    output_file.write("  #endif \\n\\\n")


def get_shaders_lib_content(shader_file, shaders_library):
    lib_content = shaders_library[SHADERS_LIB_COMMON_INDEX]
    if shader_file.find(VERTEX_SHADER_EXT) >= 0:
        lib_content += shaders_library[SHADERS_LIB_VS_INDEX]
    elif shader_file.find(FRAG_SHADER_EXT) >= 0:
        lib_content += shaders_library[SHADERS_LIB_FS_INDEX]
    return lib_content


def write_shader_line(output_file, line, convert_to_gles3, is_fragment_shader):
    if line.lstrip().startswith("//") or line == '\n' or len(line) == 0:
        return
    output_line = line.rstrip().replace(LOWP_SEARCH, "\" " + LOWP_DEFINE + " \"")
    output_line = output_line.replace(MEDIUMP_SEARCH, "\" " + MEDIUMP_DEFINE + " \"")
    output_line = output_line.replace(HIGHP_SEARCH, "\" " + HIGHP_DEFINE + " \"")
    if convert_to_gles3:
        output_line = output_line.replace("attribute", "in")
        if is_fragment_shader:
            output_line = output_line.replace("varying", "in")
        else:
            output_line = output_line.replace("varying", "out")
        output_line = output_line.replace("texture2D", "texture")
        output_line = output_line.replace("gl_FragColor", "v_FragColor")
    output_file.write("  %s \\n\\\n" % output_line)


def write_shader_body(output_file, shader_file, shader_dir, shaders_library, convert_to_gles3):
    is_fragment_shader = shader_file.find(FRAG_SHADER_EXT) >= 0
    lib_content = get_shaders_lib_content(shader_file, shaders_library)
    for line in open(os.path.join(shader_dir, shader_file)):
        if line.lstrip().startswith("void main"):
            for lib_line in lib_content.splitlines():
                write_shader_line(output_file, lib_line, convert_to_gles3, is_fragment_shader)
            if convert_to_gles3 and is_fragment_shader:
                output_file.write("  out vec4 v_FragColor; \\n\\\n")
        write_shader_line(output_file, line, convert_to_gles3, is_fragment_shader)
    output_file.write("\";\n\n")


def write_shader(output_file, shader_file, shader_dir, shaders_library):
    output_file.write("static char const %s[] = \" \\\n" % (format_shader_source_name(shader_file)))
    write_shader_gles_header(output_file)
    write_shader_body(output_file, shader_file, shader_dir, shaders_library, False)


def write_gles3_shader(output_file, shader_file, shader_dir, shaders_library):
    output_file.write("static char const %s[] = \" \\\n" % (GLES3_PREFIX + format_shader_source_name(shader_file)))
    output_file.write("  \" " + SHADER_VERSION_DEFINE + " \" \\n\\\n")
    write_shader_gles_header(output_file)
    if os.path.exists(os.path.join(shader_dir, GLES3_SHADER_PREFIX + shader_file)):
        write_shader_body(output_file, GLES3_SHADER_PREFIX + shader_file, shader_dir, shaders_library, False)
    else:
        write_shader_body(output_file, shader_file, shader_dir, shaders_library, True)


def calc_texture_slots(vertex_shader_file, fragment_shader_file, shader_dir):
    slots = set()
    for line in open(os.path.join(shader_dir, vertex_shader_file)):
        line = line.replace(" ", "")
        if line.find("uniformsampler") != -1:
            slots.add(line)
    is_inside_workaround = False;
    for line in open(os.path.join(shader_dir, fragment_shader_file)):
        line = line.replace(" ", "")  
        if line.find("#ifdefSAMSUNG_GOOGLE_NEXUS") != -1:
            is_inside_workaround = True;
            continue;
        if is_inside_workaround and line.find("#endif"):
            is_inside_workaround = False;
            continue;
        if not is_inside_workaround and line.find("uniformsampler") != -1:
            slots.add(line)
    return len(slots)


def write_shaders_index(output_file, shader_index):
    for shader in shader_index:
        output_file.write("#define %s %s\n" % (format_shader_index_name(shader), shader_index[shader]))


def write_gpu_programs_map(file, programs_def, source_prefix):
    for program in programs_def.keys():
        vertex_shader = programs_def[program][0]
        vertex_index_name = format_shader_index_name(vertex_shader)
        vertex_source_name = source_prefix + format_shader_source_name(vertex_shader)

        fragment_shader = programs_def[program][1]
        fragment_index_name = format_shader_index_name(fragment_shader)
        fragment_source_name = source_prefix + format_shader_source_name(fragment_shader)
        texture_slots = calc_texture_slots(vertex_shader, fragment_shader, shader_dir)

        file.write("    gpuIndex.insert(std::make_pair(%s, GpuProgramInfo(%s, %s, %s, %s, %d)));\n" % (
            program, vertex_index_name, fragment_index_name, vertex_source_name, fragment_source_name, texture_slots))


def write_shaders_enum(file, shader, enum_name, source_prefix):
    source_name = source_prefix + format_shader_source_name(shader)
    file.write("    %s.push_back(std::make_pair(\"%s\", std::string(%s)));\n" % (enum_name, source_name, source_name))


def write_implementation_file(programs_def, shader_index, shader_dir, impl_file, def_file, generation_dir, shaders,
                              shaders_library):
    vertex_shaders = [s for s in shaders if s.endswith(VERTEX_SHADER_EXT)]
    fragment_shaders = [s for s in shaders if s.endswith(FRAG_SHADER_EXT)]
    file = open(os.path.join(generation_dir, impl_file), 'w')
    file.write("#include \"%s\"\n\n" % (def_file))
    file.write("#include \"base/assert.hpp\"\n\n")
    file.write("#include <unordered_map>\n")
    file.write("#include <utility>\n\n")

    file.write("namespace gpu\n")
    file.write("{\n")

    for shader in shader_index.keys():
        write_shader(file, shader, shader_dir, shaders_library)
        write_gles3_shader(file, shader, shader_dir, shaders_library)

    write_shaders_index(file, shader_index)
    file.write("\n")
    for program in programs_def:
        file.write("int const %s = %s;\n" % (programs_def[program][2], program));
    file.write("\n")
    file.write("#if !defined(COMPILER_TESTS)\n")
    file.write("namespace\n")
    file.write("{\n")
    file.write("void InitGpuProgramsLib(dp::ApiVersion apiVersion, std::map<int, GpuProgramInfo> & gpuIndex)\n")
    file.write("{\n")
    file.write("  if (apiVersion == dp::ApiVersion::OpenGLES2)\n")
    file.write("  {\n")
    write_gpu_programs_map(file, programs_def, '')
    file.write("  }\n")
    file.write("  else if (apiVersion == dp::ApiVersion::OpenGLES3)\n")
    file.write("  {\n")
    write_gpu_programs_map(file, programs_def, GLES3_PREFIX)
    file.write("  }\n")
    file.write("}\n")
    file.write("}  // namespace\n\n")

    file.write("ShaderMapper::ShaderMapper(dp::ApiVersion apiVersion)\n")
    file.write("{ gpu::InitGpuProgramsLib(apiVersion, m_mapping); }\n")
    file.write("GpuProgramInfo const & ShaderMapper::GetProgramInfo(int program) const\n")
    file.write("{\n")
    file.write("  auto it = m_mapping.find(program);\n")
    file.write("  ASSERT(it != m_mapping.end(), ());\n")
    file.write("  return it->second;\n")
    file.write("}\n")
    file.write("#endif\n\n")

    file.write("#if defined(COMPILER_TESTS)\n")
    file.write("ShadersEnum GetVertexShaders(dp::ApiVersion apiVersion)\n")
    file.write("{\n")
    file.write("  ShadersEnum vertexEnum;\n")
    file.write("  if (apiVersion == dp::ApiVersion::OpenGLES2)\n")
    file.write("  {\n")
    for s in vertex_shaders:
        write_shaders_enum(file, s, 'vertexEnum', '')
    file.write("  }\n")
    file.write("  else if (apiVersion == dp::ApiVersion::OpenGLES3)\n")
    file.write("  {\n")
    for s in vertex_shaders:
        write_shaders_enum(file, s, 'vertexEnum', GLES3_PREFIX)
    file.write("  }\n")
    file.write("  return vertexEnum;\n")
    file.write("}\n")

    file.write("ShadersEnum GetFragmentShaders(dp::ApiVersion apiVersion)\n")
    file.write("{\n")
    file.write("  ShadersEnum fragmentEnum;\n")
    file.write("  if (apiVersion == dp::ApiVersion::OpenGLES2)\n")
    file.write("  {\n")
    for s in fragment_shaders:
        write_shaders_enum(file, s, 'fragmentEnum', '')
    file.write("  }\n")
    file.write("  else if (apiVersion == dp::ApiVersion::OpenGLES3)\n")
    file.write("  {\n")
    for s in fragment_shaders:
        write_shaders_enum(file, s, 'fragmentEnum', GLES3_PREFIX)
    file.write("  }\n")
    file.write("  return fragmentEnum;\n")
    file.write("}\n")
    file.write("#endif\n")

    file.write("}  // namespace gpu\n")
    file.close()


if __name__ == '__main__':
    if len(sys.argv) < 6:
        print("Usage : " + sys.argv[0] + " <shader_dir> <index_file> <shaders_lib> <generation_dir> <generated_file>")
        exit(1)

    shader_dir = sys.argv[1]
    index_file_name = sys.argv[2]
    shaders_lib_file = sys.argv[3]
    generation_dir = sys.argv[4]
    defines_file = sys.argv[5] + ".hpp"
    impl_file = sys.argv[5] + ".cpp"

    shaders = [file for file in os.listdir(shader_dir) if
               os.path.isfile(os.path.join(shader_dir, file)) and (
                   file.endswith(VERTEX_SHADER_EXT) or file.endswith(FRAG_SHADER_EXT))]
    shaderIndex = generate_shader_indexes(shaders)

    programDefinition = read_index_file(os.path.join(shader_dir, index_file_name))

    shaders_library = read_shaders_lib_file(os.path.join(shader_dir, shaders_lib_file))

    headerFile = write_definition_file(programDefinition)
    if definition_changed(headerFile, os.path.join(generation_dir, defines_file)):
        f = open(os.path.join(generation_dir, defines_file), 'w')
        f.write(headerFile)
        f.close()
    write_implementation_file(programDefinition, shaderIndex, shader_dir, impl_file, defines_file, generation_dir,
                              shaders, shaders_library)
