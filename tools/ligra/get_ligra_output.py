import sys

def get_total():
    if len(sys.argv) < 2:
        print "Usage: python get_ligra_output.py <file>"
        sys.exit(1)

    times = list()
    with open(sys.argv[1], 'r') as fin:
        for line in fin:
            results = line.split()
            time = float(results[2])
            times.append(time)
        # print all the results
        for t in times:
            print t
        print "Average_Performance: {} Min: {} Max: {}".format(sum(times) / len(times), min(times), max(times))
        with open('mean.txt', 'w') as fout:
            fout.write("MEAN={}\n".format(sum(times)/len(times)))

if __name__ == "__main__":
    get_total()
