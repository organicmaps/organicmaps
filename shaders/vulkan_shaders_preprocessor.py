import os
import re
import sys
from subprocess import Popen, PIPE
import json

VERTEX_SHADER_EXT = '.vsh.glsl'
FRAG_SHADER_EXT = '.fsh.glsl'
VERTEX_SHADER_EXT_OUT = '.vert'
FRAG_SHADER_EXT_OUT = '.frag'

SHADERS_LIB_COMMON_PATTERN = '// Common'
SHADERS_LIB_VS_PATTERN = '// VS'
SHADERS_LIB_FS_PATTERN = '// FS'
SHADERS_LIB_COMMON_INDEX = 0
SHADERS_LIB_VS_INDEX = 1
SHADERS_LIB_FS_INDEX = 2

UNIFORMS = 'uniforms'
SAMPLERS = 'samplers'

debug_output = False


# Read index file which contains program to shaders bindings.
def read_index_file(file_path):
    gpu_programs = dict()
    with open(file_path, 'r') as f:
        index = 0
        for line in f:
            line_parts = line.strip().split()
            if len(line_parts) != 3:
                print('Incorrect GPU program definition : ' + line)
                exit(1)

            vertex_shader = next(f for f in line_parts if f.endswith(VERTEX_SHADER_EXT))
            fragment_shader = next(f for f in line_parts if f.endswith(FRAG_SHADER_EXT))

            if not vertex_shader:
                print('Vertex shader not found in GPU program definition : ' + line)
                exit(1)

            if not fragment_shader:
                print('Fragment shader not found in GPU program definition : ' + line)
                exit(1)

            if line_parts[0] in gpu_programs.keys():
                print('More than one definition of %s gpu program' % line_parts[0])
                exit(1)

            gpu_programs[index] = (vertex_shader, fragment_shader, line_parts[0])
            index += 1

    gpu_programs_cache = dict()
    for (k, v) in gpu_programs.items():
        gpu_programs_cache[v[2]] = (v[0], v[1], k)

    return gpu_programs_cache


# Read GLSL-file with common shader functions.
def read_shaders_lib_file(file_path):
    shaders_library = ['', '', '']
    with open(file_path, 'r') as f:
        shaders_lib_content = f.read()
    if len(shaders_lib_content) == 0:
        return shaders_library

    common_index = shaders_lib_content.find(SHADERS_LIB_COMMON_PATTERN)
    if common_index < 0:
        print('Common functions block is not found in ' + file_path)
        exit(1)
    vs_index = shaders_lib_content.find(SHADERS_LIB_VS_PATTERN)
    if vs_index < 0:
        print('Vertex shaders functions block is not found in ' + file_path)
        exit(1)
    fs_index = shaders_lib_content.find(SHADERS_LIB_FS_PATTERN)
    if fs_index < 0:
        print('Vertex shaders functions block is not found in ' + file_path)
        exit(1)
    if not (common_index < vs_index < fs_index):
        print('Order of functions block is incorrect in ' + file_path)
        exit(1)

    shaders_library[SHADERS_LIB_COMMON_INDEX] = shaders_lib_content[common_index:vs_index - 1]
    shaders_library[SHADERS_LIB_VS_INDEX] = shaders_lib_content[vs_index:fs_index - 1]
    shaders_library[SHADERS_LIB_FS_INDEX] = shaders_lib_content[fs_index:]

    return shaders_library


def get_shaders_lib_content(shader_file, shaders_library):
    lib_content = shaders_library[SHADERS_LIB_COMMON_INDEX]
    if shader_file.find(VERTEX_SHADER_EXT) >= 0:
        lib_content += shaders_library[SHADERS_LIB_VS_INDEX]
    elif shader_file.find(FRAG_SHADER_EXT) >= 0:
        lib_content += shaders_library[SHADERS_LIB_FS_INDEX]
    return lib_content


def get_shader_line(line, layout_counters):
    if line.lstrip().startswith('//') or line == '\n' or len(line) == 0:
        return None

    output_line = line.rstrip()

    if output_line.find('layout (binding') >= 0:
        if output_line.find('sampler') >= 0:
            match = re.search(r"binding\s*=\s*(\d+)", output_line)
            sampler_match = re.search(r"sampler2D\s+(\w+)", output_line)
            if match and sampler_match:
                binding_index = int(match.group(1))
                sampler_name = sampler_match.group(1)
                if binding_index == 0:
                    print('Binding index must not be 0 for sampler in the line: ' + line)
                    exit(1)
                layout_counters[SAMPLERS][sampler_name] = binding_index
            else:
                print('Sampler name or binding index is not found in the line: ' + line)
                exit(1)
        else:
            match = re.search(r"binding\s*=\s*(\d+)", output_line)
            if match:
                binding_index = int(match.group(1))
                if binding_index != 0:
                    print('Binding index must be 0 in the line: ' + line)
                    exit(1)
            else:
                print('Binding index is not found in the line: ' + line)
                exit(1)
            layout_counters[UNIFORMS] += 1

    return output_line


def generate_spirv_compatible_glsl_shader(output_file, shader_file, shader_dir, shaders_library,
                                          layout_counters, reflection_dict):
    output_file.write('#version 310 es\n')
    output_file.write('precision highp float;\n')
    output_file.write('#define LOW_P lowp\n')
    output_file.write('#define MEDIUM_P mediump\n')
    output_file.write('#define HIGH_P highp\n')
    output_file.write('#define VULKAN_MODE\n')
    is_fragment_shader = shader_file.find(FRAG_SHADER_EXT) >= 0
    lib_content = get_shaders_lib_content(shader_file, shaders_library)
    conditional_started = False
    conditional_skip = False
    for line in open(os.path.join(shader_dir, shader_file)):
        # Remove some useless conditional compilation.
        if conditional_started and line.lstrip().startswith('#else'):
            conditional_skip = True
            continue
        if conditional_started and line.lstrip().startswith('#endif'):
            conditional_skip = False
            conditional_started = False
            continue
        if conditional_skip:
            continue
        if line.lstrip().startswith('#ifdef ENABLE_VTF'):
            conditional_started = True
            continue

        if line.lstrip().startswith('void main'):
            # Write reflection for uniforms block.
            uniforms_index = 'vs_uni';
            if is_fragment_shader:
                uniforms_index = 'fs_uni'
            if layout_counters[UNIFORMS] > 0:
                reflection_dict[uniforms_index] = 0
            else:
                reflection_dict[uniforms_index] = -1

            # Write reflection for samplers.
            sample_index = 'tex'
            if not sample_index in reflection_dict:
                reflection_dict[sample_index] = []
            for (s, idx) in layout_counters[SAMPLERS].items():
                sampler = {'name': s, 'idx': idx, 'frag':int(is_fragment_shader)}
                reflection_dict[sample_index].append(sampler)

            # Write shaders library.
            for lib_line in lib_content.splitlines():
                shader_line = get_shader_line(lib_line, layout_counters)
                if shader_line:
                    output_file.write('%s\n' % shader_line)

        shader_line = get_shader_line(line, layout_counters)
        if shader_line:
            output_file.write('%s\n' % shader_line)

    layout_counters[UNIFORMS] = 0
    layout_counters[SAMPLERS] = dict()


# Execute external program.
def execute_external(options):
    p = Popen(options, stdin=PIPE, stdout=PIPE, stderr=PIPE)
    output, err = p.communicate()
    rc = p.returncode
    if rc != 0:
        for line in err.split(b'\n'):
            print(line.decode('utf-8'))


# Generate SPIR-V shader from GLSL source.
def generate_shader(shader, shader_dir, generation_dir, shaders_library, program_name,
                    layout_counters, output_name, reflection_dict, glslc_path):
    output_path = os.path.join(generation_dir, output_name)
    with open(output_path, 'w') as file:
        generate_spirv_compatible_glsl_shader(file, shader, shader_dir, shaders_library,
                                              layout_counters, reflection_dict)
    spv_path = output_path + '.spv'
    try:
        execute_external([glslc_path, '-c', output_path, '-o', spv_path, '-std=310es', '--target-env=vulkan'])
        if debug_output:
            debug_dir = os.path.join(generation_dir, 'debug', program_name)
            debug_path = os.path.join(debug_dir, output_name)
            if not os.path.exists(debug_dir):
                os.makedirs(debug_dir)
            execute_external([glslc_path, '-S', output_path, '-o', debug_path + '.spv.txt', '-std=310es','--target-env=vulkan'])
            os.rename(output_path, debug_path)
        else:
            os.remove(output_path)
    except:
        print('Could not generate SPIR-V for the shader {0}. Most likely glslc from Android NDK is not found.'.format(shader))
        os.remove(output_path)
        exit(1)
    return spv_path


def write_shader_to_pack(pack_file, shader_file_name):
    offset = pack_file.tell()
    with open(shader_file_name, 'rb') as shader_file:
        pack_file.write(shader_file.read())
    os.remove(shader_file_name)
    return offset, pack_file.tell() - offset


if __name__ == '__main__':
    if len(sys.argv) < 7:
        print('Usage : ' + sys.argv[0] + ' <shader_dir> <index_file> <shaders_lib> <generation_dir> <glslc_path> [--debug]')
        exit(1)

    shader_dir = sys.argv[1]
    index_file_name = sys.argv[2]
    shaders_lib_file = sys.argv[3]
    generation_dir = sys.argv[4]
    glslc_path = sys.argv[5]

    if len(sys.argv) >= 7:
        debug_output = (sys.argv[6] == '--debug')

    shaders = [file for file in os.listdir(shader_dir) if
               os.path.isfile(os.path.join(shader_dir, file)) and (
                   file.endswith(VERTEX_SHADER_EXT) or file.endswith(FRAG_SHADER_EXT))]

    gpu_programs_cache = read_index_file(os.path.join(shader_dir, index_file_name))
    shaders_library = read_shaders_lib_file(os.path.join(shader_dir, shaders_lib_file))
    reflection = []
    current_offset = 0
    with open(os.path.join(generation_dir, 'shaders_pack.spv'), 'wb') as pack_file:
        for (k, v) in gpu_programs_cache.items():
            layout_counters = {UNIFORMS: 0, SAMPLERS: dict()}
            reflection_dict = {'prg': v[2], 'info': dict()}
            vs_offset = write_shader_to_pack(pack_file, generate_shader(v[0], shader_dir, generation_dir,
                                                                        shaders_library, k,
                                                                        layout_counters, k + VERTEX_SHADER_EXT_OUT,
                                                                        reflection_dict['info'], glslc_path))
            reflection_dict['vs_off'] = vs_offset[0]
            reflection_dict['vs_size'] = vs_offset[1]
            fs_offset = write_shader_to_pack(pack_file, generate_shader(v[1], shader_dir, generation_dir,
                                                                        shaders_library, k,
                                                                        layout_counters, k + FRAG_SHADER_EXT_OUT,
                                                                        reflection_dict['info'], glslc_path))
            reflection_dict['fs_off'] = fs_offset[0]
            reflection_dict['fs_size'] = fs_offset[1]
            reflection.append(reflection_dict)
    with open(os.path.join(generation_dir, 'reflection.json'), 'w') as reflection_file:
        reflection_file.write(json.dumps(reflection, separators=(',',':')))
