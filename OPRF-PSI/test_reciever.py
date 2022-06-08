import math
import ctypes
# import binascii

LONG_LONG_MAX = int('9'*19)
LONG_LONG_UNIT = int('1' + '0'*19)
DOEBULE_LONG_LONG_MAX = int('9'*(19*2 + 1))

class psi_struct(ctypes.Structure):
    _fields_ = [('length', ctypes.c_int), ('PSISet', ctypes.c_longlong)]


ll = ctypes.cdll.LoadLibrary
lib = ll("lib/libPSISO.so")
lib.runReceiver.restype = psi_struct

def c_int_array(values):
    array = ((ctypes.c_longlong*2)*len(values))()
    for index, i in enumerate(values):
        item = int(i)
        assert item <= DOEBULE_LONG_LONG_MAX
        if item > LONG_LONG_MAX:
            height = int(item // LONG_LONG_UNIT)
            low = int(item - height*LONG_LONG_UNIT)
            array[index][0] = height
            array[index][1] = low
            continue
        array[index][0] = 0
        array[index][1] = item
    return array

# c_int_array = lambda values: ((ctypes.c_longlong*2)*len(values))(*(init_array(values)))


def psi_reciever(
    raw_data,
    sender_size,
    width=609,
    log_height=None,
    hash_length_in_bytes=None,
    ip='127.0.0.1'
):
    # raw_data = [binascii.b2a_hex(str(i).encode()).decode() for i in raw_data]
    receiver_size = len(raw_data)
    print(f'python end, size: {receiver_size}')
    log_height = log_height or math.ceil(math.log2(receiver_size))
    hash_length_in_bytes = int(hash_length_in_bytes or ((log_height / 2) // 1 + 1))

    ret = lib.runReceiver(
        sender_size,
        receiver_size,
        width,
        1 << log_height,
        log_height,
        hash_length_in_bytes,
        ctypes.c_char_p(ip.encode()),
        c_int_array(raw_data)
    )
    print('python start')
    print(f'psi adder: {ret.PSISet}; length: {ret.length}')
    ptr = ctypes.POINTER((ctypes.c_longlong*2)*ret.length)
    p = ctypes.cast(ret.PSISet, ptr)
    data = [(height * LONG_LONG_UNIT + low) for low, height in p.contents]
    lib.free_heap(p)
    return data


if __name__ == '__main__':
    from uuid import uuid1
    num = 2
    raw_data = [
        str(i) for i in range(int(1.5 * 10**7), int((1.5 + num)*10**7))
    ]
    data = psi_reciever(
        raw_data=raw_data,
        sender_size=(num*10**7),
        ip='127.0.0.1',
        hash_length_in_bytes=10,
        log_height=20,
        width=621,
    )

    # raw_data = [
    #     str(i) for i in range(int(1.5*2**25), int(2.5*2**25))
    # ]
    # data = psi_reciever(
    #     raw_data=raw_data,
    #     sender_size=(2**25),
    #     ip='127.0.0.1',
    #     hash_length_in_bytes=10,
    #     log_height=25,
    #     width=800,
    # )
