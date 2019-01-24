import matplotlib.pyplot as plt

rate = 0.005

with open("perform.txt") as f:
    y = [int(x) for x in (f.read()).split()[:-1]]
    x = range(0,len(y))
    x[0] = y[0]
    for i in range(1,len(y)):
        x[i] = rate*y[i] + (1-rate)*x[i-1]
    plt.plot(xrange(len(x)), x)
    plot.xlabel("Number of games")
    plot.ylabel("Exponentially weighted average score")
    plt.show()
