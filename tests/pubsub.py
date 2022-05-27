import sys
sys.path.append("../client")

import shm_client

import threading
import time
import numpy as np

topic = "python_test_msgs"
msg_size = 4
msg_data = 5
test_msg = "shm_server test"

def publisher(client):
    client.RegisterTopic(topic)

    # Wait for subscribers
    while True:
        num_subscribers, _ = client.GetSubscriberCount(topic)
        if num_subscribers > 0:
            break

        time.sleep(0.1)

    # Allocate buffer
    buffer_name, _ = client.CreateBuffer(msg_size)

    # Map buffer and write data. We use numpy as an easy way to copy
    # data to the shm buffer
    '''
    shm = shm_client.MapBuffer(buffer_name)
    data_arr = np.ndarray((1,), dtype=np.int32, buffer=shm.buf)
    data_arr[0] = msg_data
    shm_client.UnmapBuffer(shm)
    '''
    mapfile = shm_client.MapBuffer(buffer_name)
    mapfile.write(msg_data.to_bytes(msg_size, "big"))
    shm_client.UnmapBuffer(mapfile)

    # Publish with timestamp set to 0 for this test
    metadata = test_msg.encode()
    client.Publish(topic, buffer_name, metadata, 0) # timestamp not important for this test

def subscriber(client):
    subscriber_name = "python_test_subscriber"
    client.Subscribe(topic, subscriber_name)

    buffer_name, metadata, timestamp, result = client.Pull(topic, subscriber_name)
    if result < 0:
        print(topic + " is not an active topic")
        return

    recv_msg = metadata.decode()
    if recv_msg != test_msg:
        print(f'Incorrect metadata received: {received}. Expected: {test_msg}')
        return

    size, result = client.GetBuffer(buffer_name)
    if result < 0:
        print("GetBuffer Failed")
        return

    if size != msg_size:
        print("Received msg size does not match sent msg size")
        return

    mapfile = shm_client.MapBuffer(buffer_name)
    rxData = mapfile.read()
    if int.from_bytes(rxData, "big", signed=True) == msg_data:
        print("Passed")
    else:
        print("Received data does not match sent data")

    shm_client.UnmapBuffer(mapfile)
    client.ReleaseBuffer(buffer_name)


client = shm_client.ShmClient("localhost", "50051")
publisher_thread = threading.Thread(target=publisher, args=(client,))
subscriber_thread = threading.Thread(target=subscriber, args=(client,))
publisher_thread.start()
time.sleep(0.1)
subscriber_thread.start()
publisher_thread.join()
subscriber_thread.join()
