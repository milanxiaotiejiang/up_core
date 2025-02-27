This is a core library for the up_core project,
which includes various functionalities like GPIO handling, serial communication, etc.

## 编译

```shell
./scripts/env.sh
pip install .

up build so
up build whl

up clean
```

## 查看已安装的包

```shell
pip show
```

## 串口调试

```shell
uio -r -b 1000000  /dev/ttyUSB0
```

## 启动 pup 服务

```shell
pup_core
```