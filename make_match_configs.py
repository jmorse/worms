#!/usr/bin/python

import sys

ourrange = 32

# Load a list of all teams
allteams = []
for i in range(ourrange):
    allteams.append(i)

# Format teams into a canonical order.
def order(a, b, c, d):
    out = []
    lst = [a, b, c, d]
    while lst != []:
        num = ourrange
        for i in lst:
            if i < num:
                num = i
            elif i == num:
                return ()
        lst.remove(num)
        out.append(num)
    return (out[0], out[1], out[2], out[3])

# Generate a set of unique team groupings, where there are four teams per
# match, assuming no particular order.
teamsets = set()
for i in range(ourrange):
    print >>sys.stderr, "i is {0}".format(i)
    for j in range(ourrange):
        for k in range(ourrange):
            for l in range(ourrange):
                tup = order(i, j, k, l)
                if tup == ():
                    continue
                teamsets.add(tup)

print >>sys.stderr, "Number of elements: {0}".format(len(teamsets))
for i in teamsets:
    print repr(i)
