import random
import sys

def usage():
    print('%s <number of data points>', __name__)
    sys.exit(0)

if __name__ == '__main__':

    if len(sys.argv) != 2:
        usage()

    n = int(sys.argv[1])
    for i in range(n):
        x = random.randint(2**4000, 2**6000)
        y = random.randint(2**4000, 2**6000)
        z = x * y

        print('%s,%s,%s' % (hex(x)[2:], hex(y)[2:], hex(z)[2:]))


