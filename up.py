import os
import subprocess
import sys
import argparse
import shutil
import glob


def build_shared_library():
    # 构建 C++ 共享库 (.so 文件)
    build_dir = "build"
    if os.path.exists(build_dir):
        print("Removing old build directory...")
        subprocess.check_call(f"rm -rf {build_dir}", shell=True)
        print("Old build directory removed.")

    print("Creating a new build directory...")
    os.makedirs(build_dir, exist_ok=True)
    os.chdir(build_dir)

    print("Generating build system with CMake...")
    try:
        subprocess.check_call(["cmake", ".."])
    except subprocess.CalledProcessError:
        print("CMake generation failed")
        sys.exit(1)

    print("Compiling the project...")
    try:
        subprocess.check_call(["make"])
    except subprocess.CalledProcessError:
        print("Make failed")
        sys.exit(1)

    print("Shared library (.so) build completed successfully.")
    os.chdir("..")


def build_python_package():
    # 构建 Python 包 (.whl 文件)
    print("Building Python package...")
    try:
        subprocess.check_call([sys.executable, "setup.py", "sdist", "bdist_wheel"])
    except subprocess.CalledProcessError:
        print("Python package build failed")
        sys.exit(1)
    print("Python package (.whl) built successfully.")


def clean_project():
    # 清理构建目录和 Python 包目录
    if os.path.exists("build"):
        shutil.rmtree("build")
        print("build directory cleaned up.")

    if os.path.exists("dist"):
        shutil.rmtree("dist")
        print("dist directory cleaned up.")

    if os.path.exists("pup_core.egg-info"):
        shutil.rmtree("pup_core.egg-info")
        print("pup_core.egg-info directory cleaned up.")


def main():
    parser = argparse.ArgumentParser(description="Build utility")

    # 定义子命令
    subparsers = parser.add_subparsers(dest='command')

    # clean 子命令
    subparsers.add_parser('clean', help='Clean the build and package directories.')

    # build 子命令
    build_parser = subparsers.add_parser('build', help='Build project.')
    build_parser.add_argument('target', choices=['so', 'whl'],
                              help='Target to build: so for shared library, whl for Python package.')

    args = parser.parse_args()

    # 根据不同的命令执行不同的操作
    if args.command == 'clean':
        clean_project()
    elif args.command == 'build':
        if args.target == 'so':
            build_shared_library()
        elif args.target == 'whl':
            build_python_package()


if __name__ == "__main__":
    main()
