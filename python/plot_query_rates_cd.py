#!/usr/bin/env python3

if __name__=="__main__":
    from experiment import plot
    #from experiment import plot 

    #from experiment.py import *

    import argparse,sys

    parser = argparse.ArgumentParser(description="DB Benchmark Plot Query Rates Program")
    parser.add_argument("--file", help="Specifies a file that has two columns in each row for x and y values", type=str, nargs=1)
    args = parser.parse_args()

    if args.file is None:
        print("Please specify a file")
        sys.exit()
    
    curr_plot = plot(args.file[0])
    curr_plot.plot_all()
    curr_plot.plot_one_graph()

    '''
    print("Getting here 1")
    @my_plot.plot_time_vs_cumulative_rate
    print("{}".format(my_plot.plot_all))
    print("Getting here 2")
    '''
