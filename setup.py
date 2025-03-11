from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup, find_packages
import re
import os


# 读取版本号的函数
def get_version():
    version_file = os.path.join(os.path.dirname(__file__), 'pup_core', '__init__.py')
    with open(version_file, 'r') as f:
        version_content = f.read()
    version_match = re.search(r"^__version__ = ['\"]([^'\"]*)['\"]", version_content, re.M)
    if version_match:
        return version_match.group(1)
    raise RuntimeError("Unable to find version string.")


def list_c_sources(directory):
    """递归遍历指定目录，收集所有 C/C++ 源文件的路径。

    参数：
        directory (str): 要搜索的目录路径。

    返回：
        list: 该目录及其子目录下所有 `.c` 和 `.cpp` 文件的路径列表。
    """
    sources = []
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.c') or file.endswith('.cpp') or file.endswith('.cc'):
                sources.append(os.path.join(root, file))
    return sources


# 收集 `src` 目录及其子目录下的所有 C/C++ 源文件
source_files = list_c_sources('src')

# 手动添加 `bind` 目录下的 C++ 绑定文件
source_files.append('bind/wrapper.cpp')

# 定义 Pybind11 扩展模块
ext_modules = [
    Pybind11Extension(
        "up_core",  # 生成的 Python 扩展模块名称
        source_files,  # 源文件列表
        include_dirs=[
            "include",
            "include/serial",  # 这里可能缺少逗号，导致路径拼接错误
            "include/serial/impl"
        ],
        libraries=["gpiod"],
        define_macros=[("VERSION_INFO", get_version())],  # 定义宏 VERSION_INFO
    ),
]

# 读取 README.md 作为 long_description
with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

# 配置 setuptools 的 `setup()` 函数
setup(
    name="pup_core",  # 包名称
    version=get_version(),  # 版本号
    author="noodles",  # 作者
    author_email="milanxiaotiejiang@email.com",  # 作者邮箱
    description="A core library for up_core project",  # 简短描述
    long_description=long_description,
    long_description_content_type="text/markdown",
    py_modules=['up', 'uio', 'uio_s'],  # 纯 Python 模块
    packages=find_packages(),
    install_requires=[
        'numpy',  # 依赖的第三方库
    ],  # 依赖项
    entry_points={  # 命令行工具入口
        'console_scripts': [
            'up = up:main',  # 运行 `up` 命令时，会调用 `up.py` 中的 `main()` 函数
            'uio = uio:main',
            'uio_s = uio_s:main',
            'pup_core = pup_core.__main__:cli'  # 让 `pup_core` 成为可执行命令
        ],
    },
    ext_modules=ext_modules,  # C/C++ 扩展模块
    extras_require={
        "test": "pytest"
    },  # 额外依赖项（如测试依赖）
    cmdclass={
        "build_ext": build_ext
    },  # 指定构建扩展命令
    zip_safe=False,  # 指定该项目是否可以以 zip 形式安装
    python_requires=">=3.7",  # 兼容的 Python 版本
)
