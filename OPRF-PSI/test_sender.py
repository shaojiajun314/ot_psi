import math
import ctypes

LONG_LONG_MAX = int('9'*19)
LONG_LONG_UNIT = int('1' + '0'*19)
DOEBULE_LONG_LONG_MAX = int('9'*(19*2 + 1))

ll = ctypes.cdll.LoadLibrary
lib = ll("lib/libPSISO.so")

def c_int_array(values):
    array = ((ctypes.c_longlong*2)*len(values))()
    for index, i in enumerate(values):
        item = int(i)
        assert item <= DOEBULE_LONG_LONG_MAX
        if item > LONG_LONG_MAX:
            height = int(item // LONG_LONG_UNIT)
            low = int(item - height*LONG_LONG_UNIT)
            # print(height, low)
            array[index][0] = height
            array[index][1] = low
            continue
        # print(0, item)
        array[index][0] = 0
        array[index][1] = item
    return array

# c_int_array = lambda values: (ctypes.c_longlong*len(values))(*(int(i) for i in values))


def psi_sender(
    raw_data,
    receiver_size,
    width=609,
    log_height=None,
    hash_length_in_bytes=None,
    ip='127.0.0.1'
):
    sender_size = len(raw_data)
    print(f'python end, size: {sender_size}')
    log_height = log_height or math.ceil(math.log2(receiver_size))
    hash_length_in_bytes = int(hash_length_in_bytes or ((log_height / 2) // 1 + 1))

    lib.runSender(
        sender_size,
        receiver_size,
        width,
        1 << log_height,
        log_height,
        hash_length_in_bytes,
        ctypes.c_char_p(ip.encode()),
        c_int_array(raw_data)
    )
    print('done!!!')


if __name__ == '__main__':
    from uuid import uuid1
    num = 2
    raw_data = [
        str(i) for i in range(10**7, (num + 1)*10**7)
    ]
    psi_sender(
        raw_data=raw_data,
        receiver_size=(num * 10**7),
        ip='0.0.0.0',
        hash_length_in_bytes=10,
        log_height=20,
        width=621,
    )
    # raw_data = [
    #     str(i) for i in range(2*2**25, 3*2**25)
    # ]
    # psi_sender(
    #     raw_data=raw_data,
    #     receiver_size=(2**25),
    #     ip='0.0.0.0',
    #     hash_length_in_bytes=10,
    #     log_height=25,
    #     width=800,
    # )
