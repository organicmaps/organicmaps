#!/usr/bin/env python3

import filecmp
import os
import re
import sys

LOWP_SEARCH = "lowp"
MEDIUMP_SEARCH = "mediump"
HIGHP_SEARCH = "highp"
VERTEX_SHADER_EXT = ".vsh.glsl"
FRAG_SHADER_EXT = ".fsh.glsl"

UBO_KEY = "UBO"
UNIFORMS_KEY = "Uniforms"

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
    defines_file = os.path.join(generation_dir, defines_file)
    # Write to temporary file first, and then compare if its content has changed to avoid unnecessary code rebuilds.
    defines_file_tmp = defines_file + ".tmp"
    with open(defines_file_tmp, 'w') as output_file:
        output_file.write("#pragma once\n\n")
        output_file.write("#include \"shaders/programs.hpp\"\n")
        output_file.write("#include \"shaders/gl_program_info.hpp\"\n\n")
        output_file.write("#include \"drape/drape_global.hpp\"\n\n")
        output_file.write("namespace gpu\n")
        output_file.write("{\n")
        output_file.write("extern char const * GL3_SHADER_VERSION;\n")
        output_file.write("extern char const * GLES3_SHADER_VERSION;\n\n")
        output_file.write("extern GLProgramInfo const & GetProgramInfo(dp::ApiVersion apiVersion, Program program);\n")
        output_file.write("}  // namespace gpu\n")

    if not os.path.isfile(defines_file) or not filecmp.cmp(defines_file, defines_file_tmp, False):
        os.replace(defines_file_tmp, defines_file)
        print(defines_file + " was replaced")
    else:
        os.remove(defines_file_tmp)


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


def write_shader_line(output_file, line, shader_file, binding_info):
    if line.lstrip().startswith("//") or line == '\n' or len(line) == 0:
        return False

    if line.find(LOWP_SEARCH) >= 0:
        print(f"Incorrect shader {shader_file}. Do not use lowp in shader, use LOW_P instead.")
        exit(2)
    if line.find(MEDIUMP_SEARCH) >= 0:
        print(f"Incorrect shader {shader_file}. Do not use mediump in shader, use MEDIUM_P instead.")
        exit(2)
    if line.find(HIGHP_SEARCH) >= 0:
        print(f"Incorrect shader {shader_file}. Do not use highp in shader, use HIGH_P instead.")
        exit(2)

    output_line = line.rstrip()
    
    # Extract and remove layout binding
    binding_match = re.search(r"layout\s*\(\s*binding\s*=\s*(\d+)\s*\)", output_line)
    if binding_match:
        binding_index = int(binding_match.group(1))
        # Remove the matched layout part from the string
        output_line = re.sub(r"layout\s*\(\s*binding\s*=\s*\d+\s*\)\s*", "", output_line)
    else:
        binding_index = None
        
    # Remove lauout(location = X) part. Mali compiler may not support it.
    output_line = re.sub(r"layout\s*\(\s*location\s*=\s*\d+\s*\)\s*", "", output_line)

    # Extract sampler name
    sampler_match = re.search(r"sampler2D\s+(\w+)", output_line)
    sampler_name = sampler_match.group(1) if sampler_match else None
    
    if binding_index is None and sampler_name is not None:
        print(f"Incorrect shader {shader_file}. Sampler must have binding index")
        exit(2)
    
    ubo_started = False    
    if line.find("uniform UBO") >= 0:
        if binding_index is not None:
            binding_info[shader_file].append({UBO_KEY: binding_index})
            ubo_started = True
        else:
            print(f"Incorrect shader {shader_file}. Uniform block must have binding index")
            exit(2)
        
    if binding_index and sampler_name:
        binding_info[shader_file].append({sampler_name: binding_index})
    
    if not ubo_started:    
        output_file.write("  %s \\n\\\n" % output_line)
        
    return ubo_started


def find_by_name_in_list(lst, name):
    return next((item[name] for item in lst if name in item), None)


def write_uniform_shader_line(output_file, line, shader_file, binding_info):
    if line.lstrip().startswith("//") or line == '\n' or len(line) == 0:
        return False
    output_line = line.lstrip().rstrip()
    if output_line.find("};") >= 0:
        return True
    if output_line.find("{") >= 0:
        return False
    if output_line.find(",") >= 0 or output_line.count("u_") > 1:
        print(f"Incorrect shader {shader_file}. Only one uniform per line")
        exit(2)
        
    find_by_name_in_list(binding_info[shader_file], UNIFORMS_KEY).append(output_line)
        
    output_file.write("  uniform %s \\n\\\n" % output_line)
    return False


def write_shader_body(output_file, shader_file, shader_dir, shaders_library, binding_info):
    lib_content = get_shaders_lib_content(shader_file, shaders_library)
    ubo_started = False
    for line in open(os.path.join(shader_dir, shader_file)):
        if ubo_started:
            if write_uniform_shader_line(output_file, line, shader_file, binding_info):
                ubo_started = False
            continue
        if line.lstrip().startswith("void main"):
            for lib_line in lib_content.splitlines():
                write_shader_line(output_file, lib_line, shader_file, binding_info)
        ubo_started = write_shader_line(output_file, line, shader_file, binding_info)
        if ubo_started:
            binding_info[shader_file].append({UNIFORMS_KEY: []})
    
    output_file.write("\";\n\n")


def write_shader(output_file, shader_file, shader_dir, shaders_library, binding_info):
    output_file.write("char const %s[] = \" \\\n" % format_shader_source_name(shader_file))
    write_shader_gles_header(output_file)
    write_shader_body(output_file, shader_file, shader_dir, shaders_library, binding_info)


def write_gpu_programs_map(file, programs_def, binding_info):
    for program in programs_def.keys():
        vertex_shader = programs_def[program][0]
        vertex_source_name = format_shader_source_name(vertex_shader)

        fragment_shader = programs_def[program][1]
        fragment_source_name = format_shader_source_name(fragment_shader)
        
        check_bindings(vertex_shader, fragment_shader, binding_info[vertex_shader], binding_info[fragment_shader])

        file.write("    GLProgramInfo(\"%s\", \"%s\", %s, %s),\n" % (
            vertex_source_name, fragment_source_name, vertex_source_name, fragment_source_name))


def check_bindings(vs, fs, vs_bindings, fs_bindings):
    dict1 = {k: v for d in vs_bindings for k, v in d.items()}
    dict2 = {k: v for d in fs_bindings for k, v in d.items()}
    if UBO_KEY in dict1 and UBO_KEY in dict2:
        if dict1[UBO_KEY] != dict2[UBO_KEY]:
            print(f"Shaders {vs} and {fs} must use the same binding indexes for the UBO. VS:{dict1[UBO_KEY]}, FS:{dict2[UBO_KEY]}")
            exit(2)
    if UNIFORMS_KEY in dict1 and UNIFORMS_KEY in dict2:
        if dict1[UNIFORMS_KEY] != dict2[UNIFORMS_KEY]:
            print(f"Shaders {vs} and {fs} must use the same unforms inside the UBO. VS:{dict1[UNIFORMS_KEY]}, FS:{dict2[UNIFORMS_KEY]}")
            exit(2)
    common_keys = dict1.keys() & dict2.keys()
    for key in common_keys:
        if key == UBO_KEY or key == UNIFORMS_KEY:
            continue
        if dict1[key] != dict2[key]:
            print(f"Shaders {vs} and {fs} must use the same binding indexes for textures. VS:{dict1[key]}, FS:{dict2[key]}")
            exit(2)
    
            
def write_implementation_file(programs_def, shader_index, shader_dir, impl_file, def_file, generation_dir,
                              shaders_library):
    impl_file = os.path.join(generation_dir, impl_file)
    # Write to temporary file first, and then compare if its content has changed to avoid unnecessary code rebuilds.
    impl_file_tmp = impl_file + ".tmp"
    binding_info = dict()
    with open(impl_file_tmp, 'w') as file:
        file.write("#include \"shaders/%s\"\n\n" % (def_file))
        file.write("#include \"base/assert.hpp\"\n\n")
        file.write("#include \"std/target_os.hpp\"\n\n")
        file.write("#include <array>\n\n")

        file.write("namespace gpu\n")
        file.write("{\n")
        file.write("#if defined(OMIM_OS_LINUX)\n")
        file.write("  char const * GL3_SHADER_VERSION = \"#version 310 es \\n\";\n")
        file.write("#else\n")
        file.write("  char const * GL3_SHADER_VERSION = \"#version 410 core \\n\";\n")
        file.write("#endif\n")
        file.write("char const * GLES3_SHADER_VERSION = \"#version 300 es \\n\";\n\n")

        for shader in shader_index.keys():
            binding_info[shader] = []
            write_shader(file, shader, shader_dir, shaders_library, binding_info)

        file.write("GLProgramInfo const & GetProgramInfo(dp::ApiVersion apiVersion, Program program)\n")
        file.write("{\n")
        file.write("  CHECK_EQUAL(apiVersion, dp::ApiVersion::OpenGLES3, ());\n")
        file.write("  static std::array<GLProgramInfo, static_cast<size_t>(Program::ProgramsCount)> gpuIndex = {{\n")
        write_gpu_programs_map(file, programs_def, binding_info)
        file.write("  }};\n")
        file.write("  return gpuIndex[static_cast<size_t>(program)];\n")
        file.write("}\n")
        file.write("}  // namespace gpu\n")
    
    if not os.path.isfile(impl_file) or not filecmp.cmp(impl_file, impl_file_tmp, False):
        os.replace(impl_file_tmp, impl_file)
        print(impl_file + " was replaced")
    else:
        os.remove(impl_file_tmp)


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
    shader_index = generate_shader_indexes(shaders)

    programs_order = read_programs_file(os.path.join(shader_dir, '..', programs_file_name))
    program_definition = read_index_file(os.path.join(shader_dir, index_file_name), programs_order)

    shaders_library = read_shaders_lib_file(os.path.join(shader_dir, shaders_lib_file))

    write_definition_file(defines_file, generation_dir)
    write_implementation_file(program_definition, shader_index, shader_dir, impl_file, defines_file, generation_dir,
                              shaders_library)
