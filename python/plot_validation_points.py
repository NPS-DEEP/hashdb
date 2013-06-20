#!/usr/bin/python3

import sys,os
sys.path.append(os.getenv("DOMEX_HOME") + "/src/lib/") # add the library                                           
sys.path.append("../lib/")                             # add the library      

def shrink_points(xpoints,ypoints,factor):
    newx=[]
    newy=[]
    currindex=0
    for x in range(len(xpoints)):
        if x % factor == 0:
            newx.append(xpoints[x])
            newy.append(ypoints[x])
            currindex=len(newx) - 1
        else:
            newy[currindex] =+ ypoints[x]
    return newx,newy

if __name__=="__main__":
    import argparse
    import re
    import numpy as np
    import matplotlib.pyplot as plt


    parser = argparse.ArgumentParser(description="A program to plot distribution curves")
    parser.add_argument("files", help="Specifies which file(s) plot points are stored in", type=str, nargs='+')
    parser.add_argument("--labels", help="Specifies a label for each of the plotted lines", type=str, nargs='+')
    parser.add_argument("title", help="Title of graph", type=str, nargs=1)
    parser.add_argument("xlabel", help="X axis label", type=str, nargs=1)
    parser.add_argument("ylabel", help="Y axis label", type=str, nargs=1)
    args = parser.parse_args()

    print("Args are {}".format(args))

    if args.labels != None:
        if len(args.labels) != len(args.files):
            raise RuntimeError("Number of files {} NOT EQUAL to number of labels {}".format(len(args.files),len(args.labels)))
    else:
        args.labels=[]
        # make default labels
        for x in range(len(args.files)):
            args.labels.append(str(x))

    plot_colors=['b','g','r','c','m','k']
    num_plot_colors=len(plot_colors)
        
    fig = plt.figure()
    ax = fig.add_subplot(111)

    for x in range(len(args.files)):
        print("Plotting points from {}, label is {}.".format(args.files[x],args.labels[x]))
        xpoints, ypoints = np.loadtxt(args.files[x], unpack=True)
        #ypointstotal=sum(ypoints)
        #ax.semilogy(xpoints, ypoints/ypointstotal, 'o-', label=args.labels[x], color=plot_colors[x%num_plot_colors])
        #ax.plot(xpoints, ypoints/ypointstotal, 'o-', label=args.labels[x], color=plot_colors[x%num_plot_colors])
        newx, newy = shrink_points(xpoints,ypoints,10)
        ypointstotal=sum(newy)
        ax.semilogy(newx, newy/ypointstotal, 'o-', label=args.labels[x], color=plot_colors[x%num_plot_colors])

    # make title have multiple lines
    MAX_LINE_LENGTH=40
    new_title=''
    total=0
    for word in args.title[0].split():
        if total >= MAX_LINE_LENGTH:
            new_title += '\n'
            total = 0
        new_title += word + ' '
        total += len(word) +1

    ax.set_title(new_title)
    ax.set_xlabel(args.xlabel[0])
    ax.set_ylabel(args.ylabel[0])
    ax.legend()
    fig.savefig(re.sub(r'\s', '', args.title[0])+'.png')

    
