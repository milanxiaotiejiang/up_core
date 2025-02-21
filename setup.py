import pybind11
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup
import os


def list_c_sources(directory):
    """Recursively list all C source files in the given directory."""
    sources = []
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.c') or file.endswith('.cpp'):
                sources.append(os.path.join(root, file))
    return sources


class get_pybind_include(object):
    """Helper class to determine the pybind11 include path."""

    def __str__(self):
        # Return user or local include, depending on what is available
        return pybind11.get_include(user=True)


class CustomBuildExt(build_ext):
    """Custom build extension for adding compiler-specific options."""

    def build_extensions(self):
        compiler_opts = ['-std=c11'] if self.compiler.compiler_type == 'unix' else []
        for ext in self.extensions:  # Use 'self.extensions' instead of 'self.ext_modules'
            ext.extra_compile_args.extend(compiler_opts)
        super().build_extensions()


# Collect all C source files from 'src' and its subdirectories
source_files = list_c_sources('src')
source_files.append('bind/wrapper.cpp')  # Add the pybind11 wrapper

__version__ = "0.0.1"

ext_modules = [
    Pybind11Extension(
        "up_core",
        source_files,
        include_dirs=[
            "include",
            "include/serial"
            "include/serial/impl"
        ],
        define_macros=[("VERSION_INFO", __version__)],
    ),
]

setup(
    name="up_core",
    version=__version__,
    author="noodles",
    author_email="milanxiaotiejiang@email.com",
    description="",
    long_description="",
    ext_modules=ext_modules,
    extras_require={"test": "pytest"},
    # Currently, build_ext only provides an optional "highest supported C++
    # level" feature, but in the future it may provide more features.
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.7",
)
