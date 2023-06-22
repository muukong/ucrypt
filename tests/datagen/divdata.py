import random
import sys


# x = q * y + r
def out(x, y, q, r):
    print('%s,%s,%s,%s' % (hex(x)[2:], hex(y)[2:], hex(q)[2:], hex(r)[2:]))


def edge_cases():

    # Single digit divided by single digit
    out(78, 7, 11, 1)

    # Division by single digit
    out(0x1dbdbcac1375d6423872ca869aa750511e9bd26a37c663d7dcf607894da1f1f03eb85aaff8f27381d4225b8fb60db09b560a961a4ceaa09e5e25a7005d5453428,
        7,
        3560390741284271082152764377299959095471477824662097392566463053708527716186898489600716831559524986899394435326291326600626513726195569037177043322565930,
        2)

    # Dividend smaller than divisor
    out(0xc6d49c37cbacd51bb97436046187d493a2fe1354854d2aaab78449d45dc38a26,
        0x17710b407656d14fef1e93cb875e90f6aeeab7b581972b8698468741a1f458de72295d1092e3,
        0,
        0xc6d49c37cbacd51bb97436046187d493a2fe1354854d2aaab78449d45dc38a26)

    # Self-division
    out(0x1f6c402261308e691f012fbe3ddd756a809c180ce2ea957b70fe9454652dada056f4077a8a724212e46e9c970d2b99b5b695c898fd810cecf10ca26c703b4868a,
        0x1f6c402261308e691f012fbe3ddd756a809c180ce2ea957b70fe9454652dada056f4077a8a724212e46e9c970d2b99b5b695c898fd810cecf10ca26c703b4868a,
        1,
        0)


def random_xy(n):

    for i in range(n):
        x = random.randint(2**200, 2**12000)
        y = random.randint(2**200, 2**12000)

        if x < y:
            x, y = y, x

        q = x // y
        r = x % y

        out(x, y, q, r)

def random_qry(n):

    for i in range(n):

        q = random.randint(2**200, 2**12000)
        y = random.randint(2**200, 2**12000)
        r = random.randint(2**100, y)       # ensure that q < y
        x = q * y + r

        out(x, y, q, r)





if __name__ == '__main__':

    edge_cases()
    random_xy(50)
    random_qry(50)
