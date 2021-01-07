from setuptools import setup

from distutils.extension import Extension
# Available at setup time due to pyproject.toml
from pybind11.setup_helpers import Pybind11Extension, build_ext
from pybind11 import get_cmake_dir
import platform
import sys

__version__ = "0.0.1"


# The main interface is through Pybind11Extension.
# * You can add cxx_std=11/14/17, and then build_ext can be removed.
# * You can set include_pybind11=false to add the include directory yourself,
#   say from a submodule.
#
# Note:
#   Sort input source files if you glob sources to ensure bit-for-bit
#   reproducible builds (https://github.com/pybind/python_example/pull/53)

def is_aarch64():
    machine = platform.uname()[4]
    if machine in ['aarch64']:
        return True
    else:
        return False


include_dirs = []
library_dirs = []
libraries = []
if platform.system() == 'Windows':
    include_dirs = [r'C:\opencv_44\opencv\build\include']
    library_dirs = [r'C:\opencv_44\opencv\build\x64\vc15\lib']
    # C:\opencv_44\opencv\build\x64\vc15\bin  要配到环境变量中
    libraries = ['opencv_world440', 'opencv_world440d']
elif platform.system() == 'Linux':
    if is_aarch64():
        include_dirs = [r'/usr/include/opencv4']
        library_dirs = []
        libraries = ['opencv_imgproc', 'opencv_core']
    else:
        include_dirs = []
        library_dirs = []
        libraries = ['opencv_imgproc', 'opencv_core']

ext_modules = [
    Pybind11Extension("postprocess",
                      [
                          'src/find_peaks.cpp',
                          'src/paf_score_graph.cpp',
                          'src/refine_peaks.cpp',
                          'src/munkres.cpp',
                          'src/connect_parts.cpp',
                          'src/peak.cpp',
                          'src/render_human_pose.cpp',
                          'src/human_pose.cpp',
                          'src/human_pose_estimator.cpp',
                          'src/post.cpp',
                      ],
                      include_dirs=include_dirs,
                      library_dirs=library_dirs,
                      libraries=libraries,
                      ),
]

setup(
    name="postprocess",
    version=__version__,
    author="zyx",
    description="pybind11 post",
    long_description="",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)
