#!/usr/bin/env python3
#
# Show some values for 4-bit exponent, 4-bit mantissa encoding/decoding

if __name__=="__main__":
    for i in range(1000):

        temp = i+6

        x = 0
        while temp > 15:
            x+=1
            temp //= 5

        m = temp

        # now decode
        source_count = 1
        for k in range(x):
            source_count *= 5

        source_count = (m+4) * source_count - 10

        print("before: %d   after: %d   x: %d   m:%d" % (i, source_count, x, m))

