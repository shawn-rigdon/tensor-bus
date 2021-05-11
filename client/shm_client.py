import grpc
import posix_ipc
import mmap

import sys
sys.path.append("../generated")

import batlshm_pb2
import batlshm_pb2_grpc

def MapBuffer(bufferHandle):
    shm = posix_ipc.SharedMemory(bufferHandle)
    mapfile = mmap.mmap(shm.fd, shm.size)
    shm.close_fd()
    return mapfile

def UnmapBuffer(mapfile):
    mapfile.close()

class BatlShmClient:
    def __init__(self, ip, port):
        addr = ip + ":" + port
        self.channel = grpc.insecure_channel(addr)
        self.stub = batlshm_pb2_grpc.BatlShmStub(self.channel)

    def CreateBuffer(self, size):
        request = batlshm_pb2.CreateBufferRequest(size=size)
        response = self.stub.CreateBuffer(request)
        return (response.name, response.result)

    def GetBuffer(self, name):
        request = batlshm_pb2.GetBufferRequest(name=name)
        response = self.stub.GetBuffer(request)
        return (response.size, response.result)

    def ReleaseBuffer(self, name):
        request = batlshm_pb2.ReleaseBufferRequest(name=name)
        response = self.stub.ReleaseBuffer(request)
        return response.result

    def RegisterTopic(self, name, wait=False):
        request = batlshm_pb2.RegisterTopicRequest(name=name)
        response = self.stub.RegisterTopic(request)
        while (wait and response.result == -1):
            response = self.stub.RegisterTopic(request)

        return response.result

    def Publish(self, topic_name, buffer_name, metadata, timestamp):
        request = batlshm_pb2.PublishRequest(
                topic_name=topic_name,
                buffer_name=buffer_name,
                metadata=metadata,
                timestamp=timestamp)
        response = self.stub.Publish(request)
        return response.result

    def GetSubscriberCount(self, topic_name):
        request = batlshm_pb2.SubscriberCountRequest(topic_name=topic_name)
        response = self.stub.GetSubscriberCount(request)
        return (response.num_subs, response.result)

    def Subscribe(self, topic_name, subscriber_name, depends=None, maxQueueSize=3, wait=False):
        if depends is None:
            depends = []

        request = batlshm_pb2.SubscribeRequest(topic_name=topic_name, subscriber_name=subscriber_name, maxqueuesize=maxQueueSize)
        response = self.stub.Subscribe(request)
        while (wait and response.result == -1):
            response = self.stub.Subscribe(request)

        return response.result

    def Pull(self, topic_name, subscriber_name, timeout=-1):
        request = batlshm_pb2.PullRequest(topic_name=topic_name, subscriber_name=subscriber_name, timeout=timeout)
        response = self.stub.Pull(request)
        return (response.buffer_name, response.metadata, response.timestamp, response.result)
