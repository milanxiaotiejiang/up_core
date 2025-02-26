import time
from pup_core.proto import Notification
import pytest


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


# Test functions
def test_create_notification():
    notification = create_notification(
        title="System Update",
        message="Your system will be updated at midnight.",
        sender="System Admin",
        receiver="User123"
    )
    assert notification.title == "System Update"
    assert notification.message == "Your system will be updated at midnight."
    assert notification.sender == "System Admin"
    assert notification.receiver == "User123"
    assert isinstance(notification.timestamp, int)  # Ensure timestamp is an integer


def test_serialize_notification():
    notification = create_notification(
        title="System Update",
        message="Your system will be updated at midnight.",
        sender="System Admin",
        receiver="User123"
    )
    serialized_data = serialize_notification(notification)
    assert isinstance(serialized_data, bytes)  # Ensure serialized data is in bytes


def test_deserialize_notification():
    notification = create_notification(
        title="System Update",
        message="Your system will be updated at midnight.",
        sender="System Admin",
        receiver="User123"
    )
    serialized_data = serialize_notification(notification)
    deserialized_notification = deserialize_notification(serialized_data)

    assert deserialized_notification.title == notification.title
    assert deserialized_notification.message == notification.message
    assert deserialized_notification.sender == notification.sender
    assert deserialized_notification.receiver == notification.receiver
    assert deserialized_notification.timestamp == notification.timestamp
