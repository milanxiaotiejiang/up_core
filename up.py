import os
import subprocess
import sys
import argparse
import shutil
import glob


def build_project():
    # 检查构建目录是否存在
    build_dir = "build"
    if os.path.exists(build_dir):
        # 删除旧的构建目录
        print("Removing old build directory...")
        subprocess.check_call(f"rm -rf {build_dir}", shell=True)
        print("Old build directory removed.")

    print("Creating a new build directory...")
    # 创建一个新的构建目录
    os.makedirs(build_dir, exist_ok=True)
    os.chdir(build_dir)

    print("Generating build system with CMake...")
    # 使用 CMake 生成构建系统
    try:
        subprocess.check_call(["cmake", ".."])
    except subprocess.CalledProcessError:
        print("CMake generation failed")
        sys.exit(1)

    print("Compiling the project...")
    # 编译项目
    try:
        subprocess.check_call(["make"])
    except subprocess.CalledProcessError:
        print("Make failed")
        sys.exit(1)

    print("Build completed successfully.")

    # 切换回项目根目录
    os.chdir("..")


def clean_project():
    # 清理编译目录
    if os.path.exists("build"):
        shutil.rmtree("build")
        print("build directory cleaned up.")

    # 清理 dist 目录
    if os.path.exists("dist"):
        shutil.rmtree("dist")
        print("dist directory cleaned up.")

    # 删除 up_core.egg-info 文件夹
    if os.path.exists("up_core.egg-info"):
        shutil.rmtree("up_core.egg-info")
        print("up_core.egg-info directory cleaned up.")


def main():
    parser = argparse.ArgumentParser(description="Build utility")
    parser.add_argument('command', choices=['build', 'clean', 'activate', 'deactivate'], help='Command to execute')
    args = parser.parse_args()

    if args.command == 'build':
        build_project()
    elif args.command == 'clean':
        clean_project()


if __name__ == "__main__":
    main()
