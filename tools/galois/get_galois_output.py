import sys

def get_total():
    if len(sys.argv) < 2:
        print "Usage: python get_galois_output.py <file>"
        sys.exit(1)

    num_threads = -1
    total_time = -1
    times = list()

    with open(sys.argv[1], 'r') as fin:
        for line in fin:
            if "TotalTime" not in line:
                continue
            results = line.split(",")
            num_threads = results[3]
            time = float(results[4]) / 1000
            times.append(time)
        # print all the results
        for t in times:
            print t
        print "Average_Performance: {} Min: {} Max: {}".format(sum(times) / len(times), min(times), max(times))




if __name__ == "__main__":
    get_total()
