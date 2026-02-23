project('alma', 'cpp',
  version: '0.1.0',
  default_options: ['cpp_std=c++17', 'optimization=3', 'buildtype=release']
)

openmp = dependency('openmp')

openblas_cflags = []
openblas_libs = []

openblas = dependency('openblas', required: false)
if not openblas.found()
  openblas = dependency('openblas-lib', required: false)
endif

if not openblas.found()
  message('OpenBLAS not found via pkg-config, trying common paths...')
  
  homebrew_paths = [
    '/opt/homebrew/opt/openblas',
    '/usr/local/opt/openblas',
    '/usr/opt/openblas'
  ]
  
  foreach path : homebrew_paths
    pc_file = path + '/lib/pkgconfig/openblas.pc'
    if run_command('test', '-f', pc_file, check: true).returncode() == 0
      message('Found OpenBLAS at: ' + path)
      openblas_cflags = ['-I' + path + '/include']
      openblas_libs = ['-L' + path + '/lib', '-lopenblas']
      break
    endif
  endforeach
  
  if openblas_libs.length() == 0
    error('OpenBLAS not found. Install with: apt install libopenblas-dev (Ubuntu) or brew install openblas (macOS)')
  endif
endif

add_project_arguments(['-march=native', '-ffast-math'] + openblas_cflags, language: 'cpp')

openblas_dep = []
if openblas.found()
  openblas_dep = [openblas]
endif

build_type = get_option('buildtype')
dist_dir = 'dist/' + build_type

static_lib = static_library('alma', 'src/alma.cpp',
  dependencies: [openmp] + openblas_dep,
  link_args: openblas_libs,
  install: false
)

libalma_dylib = shared_library('alma', 'src/alma.cpp',
  dependencies: [openmp] + openblas_dep,
  link_args: openblas_libs,
  install: true,
  install_dir: dist_dir + '/lib'
)

alma_exec = executable('alma', 'src/main.cpp',
  link_with: static_lib,
  dependencies: [openmp] + openblas_dep,
  link_args: openblas_libs,
  install: true,
  install_dir: dist_dir + '/exec'
)

benchmark_exec = executable('alma-benchmark', 'bench/benchmark.cpp',
  link_with: static_lib,
  dependencies: [openmp] + openblas_dep,
  link_args: openblas_libs,
  include_directories: 'src',
  install: true,
  install_dir: dist_dir + '/exec'
)

quickbench_exec = executable('quick_bench', 'bench/quick_bench.cpp',
  link_with: static_lib,
  dependencies: [openmp] + openblas_dep,
  link_args: openblas_libs,
  include_directories: 'src',
  install: true,
  install_dir: dist_dir + '/exec'
)

install_headers('src/alma.h', 'src/csv_utils.h', install_dir: dist_dir + '/include')

test_sources = [
  'tests/test_alma.cpp',
  'tests/test_util.cpp',
  'tests/test_basic.cpp',
  'tests/test_edge.cpp',
  'tests/test_random.cpp',
  'tests/test_special.cpp',
  'tests/test_large.cpp',
  'tests/test_csv.cpp'
]

test('alma', executable('test_alma', test_sources,
  link_with: static_lib,
  dependencies: [openmp] + openblas_dep,
  link_args: openblas_libs,
  include_directories: 'src'
), protocol: 'tap')
