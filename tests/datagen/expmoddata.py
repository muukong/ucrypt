import random


# z = x ** y (mod m)
def out(x, y, m, z):
    print('%s,%s,%s,%s' % (hex(x)[2:], hex(y)[2:], hex(m)[2:], hex(z)[2:]))


def edge_cases():

    # x^0 = 1 for any x (convention: 0^0 = 1)
    out(0, 0, 7, 1)
    out(7, 0, 12, 1)
    out(0x1b08d7348340be9f1ec50b15c96572a2320c9090b29685773b20c5275c80eabc2,
        0,
        0x1b08d7348340be9f1ec50b15c96572a2320c9090b29685773b20c5275c80eabc3,
        1)

    # x^1 = x for any x
    out(0, 1, 7, 0)
    out(7, 0, 8, 1)
    out(0x1b08d7348340be9f1ec50b15c96572a2320c9090b29685773b20c5275c80eabc2,
        0,
        0x1b08d7348340be9f1ec50b15c96572a2320c9090b29685773b20c52ac62beef07,
        1)


def random_data(n):

    for i in range(n):
        m = random.randint(2**256, 2**3000)
        x = random.randint(0, m)
        y = random.randint(0, m)
        z = pow(x, y, m)
        out(x, y, m, z)


if __name__ == '__main__':

    edge_cases()
    random_data(50)
