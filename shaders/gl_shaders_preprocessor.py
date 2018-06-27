import os
import re
import sys

LOWP_SEARCH = "lowp"
MEDIUMP_SEARCH = "mediump"
HIGHP_SEARCH = "highp"
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


def read_index_file(file_path, programs_order):
    gpu_programs = dict()
    with open(file_path, 'r') as f:
        index = 0
        for line in f:
            line_parts = line.strip().split()
            if len(line_parts) != 3:
                print("Incorrect GPU program definition : " + line)
                exit(10)

            if line_parts[0] != programs_order[index]:
                print("Incorrect GPU program order or name : " + line)
                exit(11)

            vertex_shader = next(f for f in line_parts if f.endswith(VERTEX_SHADER_EXT))
            fragment_shader = next(f for f in line_parts if f.endswith(FRAG_SHADER_EXT))

            if not vertex_shader:
                print("Vertex shader not found in GPU program definition : " + line)
                exit(12)

            if not fragment_shader:
                print("Fragment shader not found in GPU program definition : " + line)
                exit(13)

            if line_parts[0] in gpu_programs.keys():
                print("More than one definition of %s gpu program" % line_parts[0])
                exit(14)

            gpu_programs[index] = (vertex_shader, fragment_shader, line_parts[0])
            index += 1

    return gpu_programs


def read_programs_file(file_path):
    gpu_programs = []
    with open(file_path, 'r') as f:
        found = False
        for line in f:
            if not found and line.find('enum class Program') >= 0:
                found = True
                continue
            if found and line.find('}') >= 0:
                break
            if found and line.find('{') == -1:
                line_parts = re.split(',|=', line)
                name = line_parts[0].strip()
                if name and name != 'ProgramsCount':
                    gpu_programs.append(name)

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


def write_definition_file(defines_file, generation_dir):
    with open(os.path.join(generation_dir, defines_file), 'w') as output_file:
        output_file.write("#pragma once\n\n")
        output_file.write("#include \"shaders/programs.hpp\"\n")
        output_file.write("#include \"shaders/gl_program_info.hpp\"\n\n")
        output_file.write("#include \"drape/drape_global.hpp\"\n\n")
        output_file.write("namespace gpu\n")
        output_file.write("{\n")
        output_file.write("extern char const * GL3_SHADER_VERSION;\n")
        output_file.write("extern char const * GLES3_SHADER_VERSION;\n\n")
        output_file.write("extern GLProgramInfo GetProgramInfo(dp::ApiVersion apiVersion, Program program);\n")
        output_file.write("}  // namespace gpu\n")


def write_shader_gles_header(output_file):
    output_file.write("  #ifdef GL_ES \\n\\\n")
    output_file.write("    #ifdef GL_FRAGMENT_PRECISION_HIGH \\n\\\n")
    output_file.write("      #define MAXPREC highp \\n\\\n")
    output_file.write("    #else \\n\\\n")
    output_file.write("      #define MAXPREC mediump \\n\\\n")
    output_file.write("    #endif \\n\\\n")
    output_file.write("    precision MAXPREC float; \\n\\\n")
    output_file.write("    #define LOW_P lowp \\n\\\n")
    output_file.write("    #define MEDIUM_P mediump \\n\\\n")
    output_file.write("    #define HIGH_P highp \\n\\\n")
    output_file.write("  #else \\n\\\n")
    output_file.write("    #define LOW_P \\n\\\n")
    output_file.write("    #define MEDIUM_P \\n\\\n")
    output_file.write("    #define HIGH_P \\n\\\n")
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

    if line.find(LOWP_SEARCH) >= 0:
        print("Incorrect shader. Do not use lowp in shader, use LOW_P instead.")
        exit(2)
    if line.find(MEDIUMP_SEARCH) >= 0:
        print("Incorrect shader. Do not use mediump in shader, use MEDIUM_P instead.")
        exit(2)
    if line.find(HIGHP_SEARCH) >= 0:
        print("Incorrect shader. Do not use highp in shader, use HIGH_P instead.")
        exit(2)

    output_line = line.rstrip()
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
    output_file.write("char const %s[] = \" \\\n" % (format_shader_source_name(shader_file)))
    write_shader_gles_header(output_file)
    write_shader_body(output_file, shader_file, shader_dir, shaders_library, False)


def write_gles3_shader(output_file, shader_file, shader_dir, shaders_library):
    output_file.write("char const %s[] = \" \\\n" % (GLES3_PREFIX + format_shader_source_name(shader_file)))
    write_shader_gles_header(output_file)
    if os.path.exists(os.path.join(shader_dir, GLES3_SHADER_PREFIX + shader_file)):
        write_shader_body(output_file, GLES3_SHADER_PREFIX + shader_file, shader_dir, shaders_library, False)
    else:
        write_shader_body(output_file, shader_file, shader_dir, shaders_library, True)


def write_gpu_programs_map(file, programs_def, source_prefix):
    for program in programs_def.keys():
        vertex_shader = programs_def[program][0]
        vertex_source_name = source_prefix + format_shader_source_name(vertex_shader)

        fragment_shader = programs_def[program][1]
        fragment_source_name = source_prefix + format_shader_source_name(fragment_shader)

        file.write("      GLProgramInfo(\"%s\", \"%s\", %s, %s),\n" % (
            vertex_source_name, fragment_source_name, vertex_source_name, fragment_source_name))


def write_implementation_file(programs_def, shader_index, shader_dir, impl_file, def_file, generation_dir,
                              shaders_library):
    with open(os.path.join(generation_dir, impl_file), 'w') as file:
        file.write("#include \"shaders/%s\"\n\n" % (def_file))
        file.write("#include \"base/assert.hpp\"\n\n")
        file.write("#include \"std/target_os.hpp\"\n\n")
        file.write("#include <array>\n\n")

        file.write("namespace gpu\n")
        file.write("{\n")
        file.write("char const * GL3_SHADER_VERSION = \"#version 150 core \\n\";\n")
        file.write("char const * GLES3_SHADER_VERSION = \"#version 300 es \\n\";\n\n")

        for shader in shader_index.keys():
            write_shader(file, shader, shader_dir, shaders_library)
            write_gles3_shader(file, shader, shader_dir, shaders_library)

        file.write("GLProgramInfo GetProgramInfo(dp::ApiVersion apiVersion, Program program)\n")
        file.write("{\n")
        file.write("  if (apiVersion == dp::ApiVersion::OpenGLES2)\n")
        file.write("  {\n")
        file.write("    static std::array<GLProgramInfo, static_cast<size_t>(Program::ProgramsCount)> gpuIndex = {{\n")
        write_gpu_programs_map(file, programs_def, '')
        file.write("    }};\n")
        file.write("    return gpuIndex[static_cast<size_t>(program)];\n")
        file.write("  }\n")
        file.write("  else if (apiVersion == dp::ApiVersion::OpenGLES3)\n")
        file.write("  {\n")
        file.write("    static std::array<GLProgramInfo, static_cast<size_t>(Program::ProgramsCount)> gpuIndex = {{\n")
        write_gpu_programs_map(file, programs_def, GLES3_PREFIX)
        file.write("    }};\n")
        file.write("    return gpuIndex[static_cast<size_t>(program)];\n")
        file.write("  }\n")
        file.write("  CHECK(false, (\"Unsupported API version.\"));\n")
        file.write("  return {};\n")
        file.write("}\n")
        file.write("}  // namespace gpu\n")


if __name__ == '__main__':
    if len(sys.argv) < 6:
        print("Usage : " + sys.argv[0] + " <shader_dir> <index_file> <programs_file> <shaders_lib> <generation_dir> <generated_file>")
        exit(1)

    shader_dir = sys.argv[1]
    index_file_name = sys.argv[2]
    programs_file_name = sys.argv[3]
    shaders_lib_file = sys.argv[4]
    generation_dir = sys.argv[5]
    defines_file = sys.argv[6] + ".hpp"
    impl_file = sys.argv[6] + ".cpp"

    shaders = [file for file in os.listdir(shader_dir) if
               os.path.isfile(os.path.join(shader_dir, file)) and (
                   file.endswith(VERTEX_SHADER_EXT) or file.endswith(FRAG_SHADER_EXT))]
    shaderIndex = generate_shader_indexes(shaders)

    programs_order = read_programs_file(os.path.join(shader_dir, '..', programs_file_name))
    programDefinition = read_index_file(os.path.join(shader_dir, index_file_name), programs_order)

    shaders_library = read_shaders_lib_file(os.path.join(shader_dir, shaders_lib_file))

    write_definition_file(defines_file, generation_dir)
    write_implementation_file(programDefinition, shaderIndex, shader_dir, impl_file, defines_file, generation_dir,
                              shaders_library)
