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

IN = 'in'
OUT = 'out'
UNIFORMS = 'uniforms'
SAMPLERS = 'samplers'

debug_output = False


# Read index file which contains program to shaders bindings.
def read_index_file(file_path, programs_order):
    gpu_programs = dict()
    with open(file_path, 'r') as f:
        index = 0
        for line in f:
            line_parts = line.strip().split()
            if len(line_parts) != 3:
                print('Incorrect GPU program definition : ' + line)
                exit(1)

            if line_parts[0] != programs_order[index]:
                print('Incorrect GPU program order or name : ' + line)
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


# Read hpp-file with programs enumeration.
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


def drop_variable_initialization(line):
    equal_found = line.find('=')
    if equal_found:
        return line[:equal_found - 1]
    return line.replace(';', '')


def get_program_param(line):
    glsl_found = line.find('glsl::')
    if glsl_found >= 0:
        return drop_variable_initialization(line[glsl_found + 6:].replace('m_', 'u_'))
    if line.find('float ') >= 0 or line.find('int ') >= 0:
        return drop_variable_initialization(line.lstrip().replace('m_', 'u_'))
    return None


def get_program(line):
    program_found = line.find('Program::')
    if program_found >= 0:
        return line[program_found + 9:].replace(',', '').replace(')', '').replace('\n', '')
    return None


# Read hpp-file with program parameters declaration.
def read_program_params_file(file_path):
    program_params = []
    programs = []
    result = dict()
    with open(file_path, 'r') as f:
        block_found = False
        for line in f:
            if line.find('struct') >= 0 and line.find('ProgramParams') >= 0:
                block_found = True
                program_params = []
                programs = []
                continue
            if block_found and line.find('}') >= 0:
                block_found = False
                for p in programs:
                    result[p] = program_params
                continue
            if block_found:
                param = get_program_param(line)
                if param:
                    program_params.append(param.split(' '))
                program = get_program(line)
                if program:
                    programs.append(program)
    return result


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


def get_shader_line(line, layout_counters, is_fragment_shader):
    if line.lstrip().startswith('//') or line == '\n' or len(line) == 0:
        return None

    output_line = line.rstrip()

    if output_line.find('uniform ') >= 0:
        if output_line.find('sampler') >= 0:
            layout_counters[SAMPLERS][1].append(output_line)
        else:
            layout_counters[UNIFORMS] += 1
        return None

    if output_line.find('attribute ') >= 0:
        location = layout_counters[IN]
        layout_counters[IN] += 1
        output_line = output_line.replace('attribute', 'layout (location = {0}) in'.format(location))

    if output_line.find('varying ') >= 0:
        if is_fragment_shader:
            location = layout_counters[IN]
            layout_counters[IN] += 1
            output_line = output_line.replace('varying', 'layout (location = {0}) in'.format(location))
        else:
            location = layout_counters[OUT]
            layout_counters[OUT] += 1
            output_line = output_line.replace('varying', 'layout (location = {0}) out'.format(location))
    output_line = output_line.replace('texture2D', 'texture')
    output_line = output_line.replace('gl_FragColor', 'v_FragColor')
    return output_line


def get_size_by_type(type):
    if type == 'float' or type == 'int':
        return 1
    if type == 'vec2':
        return 2
    if type == 'vec3':
        return 3
    if type == 'vec4':
        return 4
    if type == 'mat4':
        return 16
    print('Type is not supported' + type)
    exit(1)


def get_subscript(offset, param):
    symbols = ['x', 'y', 'z', 'w']
    subscript = ''
    for i in range(0, get_size_by_type(param[0])):
        subscript += symbols[offset + i]
    return subscript


def write_uniform_block(output_file, program_params):
    groups = []
    c = 0
    group_index = 0
    group_params = []
    for p in program_params:
        sz = get_size_by_type(p[0])
        if sz % 4 == 0:
            groups.append((p[0], p[1], [p]))
        else:
            if c + sz < 4:
                group_params.append(p)
                c += sz
            elif c + sz == 4:
                group_params.append(p)
                groups.append(('vec4', 'u_grouped{0}'.format(group_index), group_params))
                group_index += 1
                group_params = []
                c = 0
            else:
                print('Must be possible to unite sequential variables to vec4')
                exit(1)
    if c != 0:
        groups.append(('vec4', 'u_grouped{0}'.format(group_index), group_params))

    output_file.write('layout (binding = 0) uniform UBO\n')
    output_file.write('{\n')
    for g in groups:
        output_file.write('  {0} {1};\n'.format(g[0], g[1]))
    output_file.write('} uniforms;\n')
    for k in groups:
        name = k[1]
        params = k[2]
        offset = 0
        if len(params) == 1 and get_size_by_type(params[0][0]) % 4 == 0:
            output_file.write('#define {0} uniforms.{1}\n'.format(params[0][1], name))
            continue
        for param in params:
            output_file.write('#define {0} uniforms.{1}.{2}\n'.format(param[1], name, get_subscript(offset, param)))
            offset += get_size_by_type(param[0])


def get_size_of_attributes_block(lines_before_main):
    for i, line in reversed(list(enumerate(lines_before_main))):
        if line.find('layout (location') >= 0:
            return i + 1
    return len(lines_before_main)


def generate_spirv_compatible_glsl_shader(output_file, shader_file, shader_dir, shaders_library,
                                          program_params, layout_counters, reflection_dict):
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
    lines_before_main = []
    main_found = False
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
        if line.lstrip().startswith('#ifdef ENABLE_VTF') or line.lstrip().startswith('#ifdef GLES3'):
            conditional_started = True
            continue

        if line.lstrip().startswith('void main'):
            main_found = True

            # Write attributes.
            sz = get_size_of_attributes_block(lines_before_main)
            for i in range(0, sz):
                output_file.write('%s\n' % lines_before_main[i])

            if is_fragment_shader:
                output_file.write('layout (location = 0) out vec4 v_FragColor;\n')

            # Write uniforms block.
            uniforms_index = 'vs_uni';
            if is_fragment_shader:
                uniforms_index = 'fs_uni'
            if layout_counters[UNIFORMS] > 0:
                write_uniform_block(output_file, program_params)
                reflection_dict[uniforms_index] = 0
            else:
                reflection_dict[uniforms_index] = -1

            # Write samplers.
            sample_index = 'tex'
            samplers_offset = layout_counters[SAMPLERS][0]
            if layout_counters[UNIFORMS] > 0 and samplers_offset == 0:
                samplers_offset = 1
            for idx, s in enumerate(layout_counters[SAMPLERS][1]):
                output_file.write('layout (binding = {0}) {1}\n'.format(samplers_offset + idx, s))
                sampler = {'name': s[s.find('u_'):-1], 'idx': samplers_offset + idx, 'frag':int(is_fragment_shader)}
                if not sample_index in reflection_dict:
                    reflection_dict[sample_index] = [sampler]
                else:
                    reflection_dict[sample_index].append(sampler)
            layout_counters[SAMPLERS][0] = samplers_offset + len(layout_counters[SAMPLERS][1])
            layout_counters[SAMPLERS][1] = []

            # Write shaders library.
            for lib_line in lib_content.splitlines():
                shader_line = get_shader_line(lib_line, layout_counters, is_fragment_shader)
                if shader_line:
                    output_file.write('%s\n' % shader_line)

            # Write rest lines.
            for i in range(sz, len(lines_before_main)):
                output_file.write('%s\n' % lines_before_main[i])

        shader_line = get_shader_line(line, layout_counters, is_fragment_shader)
        if not shader_line:
            continue

        if main_found:
            output_file.write('%s\n' % shader_line)
        else:
            lines_before_main.append(shader_line)
    layout_counters[IN] = 0
    layout_counters[OUT] = 0
    layout_counters[UNIFORMS] = 0


# Execute external program.
def execute_external(options):
    p = Popen(options, stdin=PIPE, stdout=PIPE, stderr=PIPE)
    output, err = p.communicate()
    rc = p.returncode
    if rc != 0:
        for line in err.split(b'\n'):
            print(line.decode('utf-8'))


# Generate SPIR-V shader from GLSL source.
def generate_shader(shader, shader_dir, generation_dir, shaders_library, program_name, program_params,
                    layout_counters, output_name, reflection_dict, glslc_path):
    output_path = os.path.join(generation_dir, output_name)
    with open(output_path, 'w') as file:
        generate_spirv_compatible_glsl_shader(file, shader, shader_dir, shaders_library,
                                              program_params, layout_counters, reflection_dict)
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


# Check if varying are in the same order in vertex and fragment shaders.
def check_varying_consistency(vs_file_name, fs_file_name):
    vs_varyings = []
    for line in open(vs_file_name):
        line = line.lstrip().rstrip()
        if line.startswith('varying '):
            vs_varyings.append(line)
    fs_varyings = []
    for line in open(fs_file_name):
        line = line.lstrip().rstrip()
        if line.startswith('varying '):
            fs_varyings.append(line)
    return vs_varyings == fs_varyings


def write_shader_to_pack(pack_file, shader_file_name):
    offset = pack_file.tell()
    with open(shader_file_name, 'rb') as shader_file:
        pack_file.write(shader_file.read())
    os.remove(shader_file_name)
    return offset, pack_file.tell() - offset


if __name__ == '__main__':
    if len(sys.argv) < 7:
        print('Usage : ' + sys.argv[0] + ' <shader_dir> <index_file> <programs_file> <program_params_file> <shaders_lib> <generation_dir> <glslc_path> [--debug]')
        exit(1)

    shader_dir = sys.argv[1]
    index_file_name = sys.argv[2]
    programs_file_name = sys.argv[3]
    program_params_file_name = sys.argv[4]
    shaders_lib_file = sys.argv[5]
    generation_dir = sys.argv[6]
    glslc_path = sys.argv[7]

    if len(sys.argv) >= 9:
        debug_output = (sys.argv[8] == '--debug')

    shaders = [file for file in os.listdir(shader_dir) if
               os.path.isfile(os.path.join(shader_dir, file)) and (
                   file.endswith(VERTEX_SHADER_EXT) or file.endswith(FRAG_SHADER_EXT))]

    programs_order = read_programs_file(os.path.join(shader_dir, '..', programs_file_name))
    program_params = read_program_params_file(os.path.join(shader_dir, '..', program_params_file_name))
    gpu_programs_cache = read_index_file(os.path.join(shader_dir, index_file_name), programs_order)
    shaders_library = read_shaders_lib_file(os.path.join(shader_dir, shaders_lib_file))
    reflection = []
    current_offset = 0
    with open(os.path.join(generation_dir, 'shaders_pack.spv'), 'wb') as pack_file:
        for (k, v) in gpu_programs_cache.items():
            if not k in program_params:
                print('Program params were not found for the shader ' + k)
                exit(1)
            if not check_varying_consistency(os.path.join(shader_dir, v[0]), os.path.join(shader_dir, v[1])):
                print('Varyings must be in the same order in VS and FS. Shaders: {0}, {1} / Program: {2}.'.format(v[0], v[1], k))
                exit(1)
            layout_counters = {IN: 0, OUT: 0, UNIFORMS: 0, SAMPLERS: [0, list()]}
            reflection_dict = {'prg': v[2], 'info': dict()}
            vs_offset = write_shader_to_pack(pack_file, generate_shader(v[0], shader_dir, generation_dir,
                                                                        shaders_library, k, program_params[k],
                                                                        layout_counters, k + VERTEX_SHADER_EXT_OUT,
                                                                        reflection_dict['info'], glslc_path))
            reflection_dict['vs_off'] = vs_offset[0]
            reflection_dict['vs_size'] = vs_offset[1]
            fs_offset = write_shader_to_pack(pack_file, generate_shader(v[1], shader_dir, generation_dir,
                                                                        shaders_library, k, program_params[k],
                                                                        layout_counters, k + FRAG_SHADER_EXT_OUT,
                                                                        reflection_dict['info'], glslc_path))
            reflection_dict['fs_off'] = fs_offset[0]
            reflection_dict['fs_size'] = fs_offset[1]
            reflection.append(reflection_dict)
    with open(os.path.join(generation_dir, 'reflection.json'), 'w') as reflection_file:
        reflection_file.write(json.dumps(reflection, separators=(',',':')))
