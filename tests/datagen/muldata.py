import random
import sys


def out(x, y, z):

    print('%s,%s,%s' % (hex(x)[2:], hex(y)[2:], hex(z)[2:]))

def edge_cases():

    # Single digit multiplication
    out(0x7, 0x8, 0x38)

    # Multiplication by zero
    out(0x0, 0x0, 0x0)
    out(0x1, 0x0, 0x0)
    out(0x7, 0x0, 0x0)
    out(0x1fdcad737766e5def7c9f20855a0c32b7dd68dd2f98960500b965196980bdd820a6cc815e72d1b46001b79217b98129cc2e8ca7377a103296c973b6d1a11345b0,
        0,
        0)

    # Multiplication by one
    out(0x7, 0x1, 0x7)
    out(0x1fdcad737766e5def7c9f20855a0c32b7dd68dd2f98960500b965196980bdd820a6cc815e72d1b46001b79217b98129cc2e8ca7377a103296c973b6d1a11345b0,
        1,
        0x1fdcad737766e5def7c9f20855a0c32b7dd68dd2f98960500b965196980bdd820a6cc815e72d1b46001b79217b98129cc2e8ca7377a103296c973b6d1a11345b0)


def random_cases(n):

    for i in range(n):

        x = random.randint(0, 2**12000)
        y = random.randint(0, 2**12000)
        z = x * y

        out(x, y, z)


if __name__ == '__main__':

    edge_cases()
    random_cases(200)


