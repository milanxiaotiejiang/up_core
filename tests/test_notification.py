import time
from pup_core.proto import Notification


def create_notification(title: str, message: str, sender: str, receiver: str):
    notification = Notification(
        title=title,
        message=message,
        sender=sender,
        receiver=receiver,
        timestamp=int(time.time())
    )
    return notification


def serialize_notification(notification):
    """将 Notification 对象序列化为二进制格式"""
    return notification.SerializeToString()


def deserialize_notification(serialized_data):
    """将二进制数据反序列化为 Notification 对象"""
    notification = Notification()
    notification.ParseFromString(serialized_data)
    return notification


# 示例使用
notification = create_notification(
    title="System Update",
    message="Your system will be updated at midnight.",
    sender="System Admin",
    receiver="User123"
)

# 打印通知内容
print(f"Notification:\n"
      f"Title: {notification.title}\n"
      f"Message: {notification.message}\n"
      f"Sender: {notification.sender}\n"
      f"Receiver: {notification.receiver}\n"
      f"Timestamp: {notification.timestamp}")

# 序列化通知对象
serialized_data = serialize_notification(notification)

# 反序列化通知对象
deserialized_notification = deserialize_notification(serialized_data)

# 打印反序列化后的通知内容
print(f"\nDeserialized Notification:\n"
      f"Title: {deserialized_notification.title}\n"
      f"Message: {deserialized_notification.message}\n"
      f"Sender: {deserialized_notification.sender}\n"
      f"Receiver: {deserialized_notification.receiver}\n"
      f"Timestamp: {deserialized_notification.timestamp}")
