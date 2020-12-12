import grpc
from multiprocessing import shared_memory

import sys
sys.path.append("../generated")

import batlshm_pb2
import batlshm_pb2_grpc

def MapBuffer(buffer_name):
    print("BUFFER NAME: " + buffer_name)
    return shared_memory.SharedMemory(buffer_name)

def UnmapBuffer(shm):
    shm.close()

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

    def RegisterTopic(self, name):
        request = batlshm_pb2.RegisterTopicRequest(name=name)
        response = self.stub.RegisterTopic(request)
        return response.result

    def Publish(self, topic_name, buffer_name, timestamp):
        request = batlshm_pb2.PublishRequest(
                topic_name=topic_name,
                buffer_name=buffer_name,
                timestamp=timestamp)
        response = self.stub.Publish(request)
        return response.result

    def GenerateID(self):
        request = batlshm_pb2.Empty()
        response = self.stub.GenerateID(request)
        return response.id

    def Subscribe(self, topic_name, ID):
        request = batlshm_pb2.SubscribeRequest(topic_name=topic_name, id=ID)
        response = self.stub.Subscribe(request)
        return response.result

    def Pull(self, topic_name, ID):
        request = batlshm_pb2.PullRequest(topic_name=topic_name, id=ID)
        response = self.stub.Pull(request)
        return (response.buffer_name, response.timestamp, response.result)
