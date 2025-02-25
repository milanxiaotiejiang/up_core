import uvicorn


def run_server(host: str = '0.0.0.0', port: int = 8000, reload: bool = False, log_level: str = 'info'):
    """启动 Uvicorn 服务器，支持传入多个参数"""
    uvicorn.run(
        "pup_core.app:app",  # 使用模块导入字符串，而不是直接传递 app 实例
        host=host,
        port=port,
        reload=reload,
        log_level=log_level,
        use_colors=False
    )
