project('alma', 'cpp',
  version: '0.2.0',
  default_options: ['cpp_std=c++17', 'optimization=3', 'buildtype=release']
)

static_link = get_option('static-link')
enable_static = static_link or get_option('default_library') == 'static'

openmp = dependency('openmp')

lapacke = dependency('lapacke', required: false)
if not lapacke.found()
  lapacke = cc.find_library('lapacke', required: true)
endif

blas_lib = ''
blas_cflags = []
blas_libs = []
blas_dep = []
use_mkl = false
use_accelerate = false
backend_name = 'openblas'

mkl = dependency('mkl', required: false)
if mkl.found()
  message('Using Intel MKL')
  blas_dep = [mkl]
  use_mkl = true
  backend_name = 'mkl'
else
  mkl_core = dependency('mkl_core', required: false)
  if mkl_core.found()
    message('Using Intel MKL (core)')
    blas_dep = [mkl_core]
    use_mkl = true
    backend_name = 'mkl'
  endif
endif

if not use_mkl
  blas = dependency('blas', required: false)
  if blas.found()
    foreach lib : ['openblas', 'blis', 'atlas', 'goto']
      if lib == 'openblas'
        openblas = dependency('openblas', required: false)
        if not openblas.found()
          openblas = dependency('openblas-lib', required: false)
        endif
        if openblas.found()
          message('Using OpenBLAS')
          if enable_static
            blas_libs = ['-static', '-lopenblas', '-lpthread']
            message('Static linking enabled')
          else
            blas_dep = [openblas]
          endif
          backend_name = 'openblas'
          break
        endif
      else
        libdep = dependency(lib, required: false)
        if libdep.found()
          message('Using @0@'.format(lib))
          if enable_static
            blas_libs = ['-static', '-l' + lib, '-lpthread']
          else
            blas_dep = [libdep]
          endif
          backend_name = lib
          break
        endif
      endif
    endforeach
  endif
endif

if blas_dep.length() == 0
  message('No BLAS found via pkg-config, trying common paths...')
  
  homebrew_paths = [
    '/opt/homebrew/opt/openblas',
    '/usr/local/opt/openblas',
    '/usr/opt/openblas'
  ]
  
  foreach path : homebrew_paths
    pc_file = path + '/lib/pkgconfig/openblas.pc'
    if run_command('test', '-f', pc_file, check: true).returncode() == 0
      message('Found OpenBLAS at: ' + path)
      blas_cflags = ['-I' + path + '/include']
      blas_libs = ['-L' + path + '/lib', '-lopenblas']
      backend_name = 'openblas'
      break
    endif
  endforeach
  
if blas_libs.length() == 0 and blas_dep.length() == 0
  message('No BLAS found via pkg-config, trying common paths...')
  
  homebrew_paths = [
    '/opt/homebrew/opt/openblas',
    '/usr/local/opt/openblas',
    '/usr/opt/openblas'
  ]
  
  foreach path : homebrew_paths
    pc_file = path + '/lib/pkgconfig/openblas.pc'
    if run_command('test', '-f', pc_file, check: true).returncode() == 0
      message('Found OpenBLAS at: ' + path)
      if enable_static
        blas_cflags = ['-I' + path + '/include']
        blas_libs = ['-static', '-L' + path + '/lib', '-lopenblas', '-lpthread']
        message('Static linking enabled')
      else
        blas_cflags = ['-I' + path + '/include']
        blas_libs = ['-L' + path + '/lib', '-lopenblas']
      endif
      backend_name = 'openblas'
      break
    endif
  endforeach
  
  if blas_libs.length() == 0
    error('No BLAS library found. Install OpenBLAS or MKL')
  endif
endif
endif

opt_args = []

cpu_family = host_machine.cpu_family()
system = host_machine.system()

if system == 'darwin'
  message('macOS detected, checking for Accelerate...')
  accelerate = dependency('accelerate', required: false)
  if accelerate.found()
    message('Using Apple Accelerate framework')
    blas_dep = [accelerate]
    use_accelerate = true
    backend_name = 'accelerate'
  endif
  opt_args += ['-march=native', '-O3']
elif system == 'linux'
  opt_args += ['-march=native', '-ffast-math', '-funroll-loops']
  
  if cpu_family == 'x86_64'
    opt_args += ['-mfma', '-mavx', '-mavx2']
  elif cpu_family == 'aarch64' or cpu_family == 'arm64'
    opt_args += ['-march=armv8.2-a+dotprod']
  endif
endif

if use_mkl
  opt_args += ['-DMKL_LP64', '-DMKL_DYNAMIC']
endif

add_project_arguments(opt_args, language: 'cpp')

message('Backend: ' + backend_name)
message('CPU: ' + cpu_family)
message('System: ' + system)

build_type = get_option('buildtype')
dist_dir = 'dist/' + build_type

platform_sources = []
if system == 'linux'
  platform_sources = ['src/core/platform/alma_linux.cpp']
elif system == 'darwin'
  platform_sources = ['src/core/platform/alma_macos.cpp']
endif

static_lib = static_library('alma', ['src/core/alma.cpp'] + platform_sources,
  include_directories: ['src', 'src/include'],
  dependencies: [openmp, lapacke] + blas_dep,
  link_args: blas_libs,
  install: false
)

libalma_dylib = shared_library('alma', ['src/core/alma.cpp'] + platform_sources,
  include_directories: ['src', 'src/include'],
  dependencies: [openmp, lapacke] + blas_dep,
  link_args: blas_libs,
  install: true,
  install_dir: dist_dir + '/lib'
)

alma_exec = executable('alma', 'src/cli/main.cpp',
  include_directories: ['src', 'src/include'],
  link_with: static_lib,
  dependencies: [openmp, lapacke] + blas_dep,
  link_args: blas_libs,
  install: true,
  install_dir: dist_dir + '/exec'
)

benchmark_exec = executable('alma-benchmark', 'bench/benchmark.cpp',
  include_directories: ['src', 'src/include'],
  link_with: static_lib,
  dependencies: [openmp, lapacke] + blas_dep,
  link_args: blas_libs,
  install: true,
  install_dir: dist_dir + '/exec'
)

quickbench_exec = executable('quick_bench', 'bench/quick_bench.cpp',
  include_directories: ['src', 'src/include'],
  link_with: static_lib,
  dependencies: [openmp, lapacke] + blas_dep,
  link_args: blas_libs,
  install: true,
  install_dir: dist_dir + '/exec'
)

install_headers('src/include/alma/alma.h', 'src/include/alma/alma_cache.h', 'src/io/csv_utils.h', install_dir: dist_dir + '/include')

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
  dependencies: [openmp, lapacke] + blas_dep,
  link_args: blas_libs,
  include_directories: ['src', 'src/include']
), protocol: 'tap')
