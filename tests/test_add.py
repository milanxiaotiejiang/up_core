import up_core as up


def test_add():
    # 测试add函数
    result = up.add(5, 3)
    assert result == 8, f"Expected 8, but got {result}"
    print("test_add passed")


def test_system():
    # 测试system函数
    up.system("ls")


if __name__ == "__main__":
    # 运行测试用例
    test_add()
    test_system()
