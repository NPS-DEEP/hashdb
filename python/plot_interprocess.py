#!/usr/bin/env python2

"""
Author: Joel Young <jdyoung@nps.edu>

Reads output files genned by test_interprocess script and plots
transactions completed vs seconds required

Execute:
To plot r0.xml, r3.xml, and r4.xml use:
./plot_interprocess.py 0 3 4

You can change the order as you like:
./plot_interprocess.py 3 0 4

Because of the color_map, you can currently only plot 7 results in one figure.
"""

if __name__=="__main__":
    import argparse 
    import sys,time

    import xml.dom.minidom

    import matplotlib
    import matplotlib.pyplot as plt
    import matplotlib.mlab   as mlab
    from mpl_toolkits.axes_grid1 import make_axes_locatable

    color_map = {0:'b'                 # the color for each plot
                ,1:'g'
                ,2:'r'
                ,3:'y'
                ,4:'m'
                ,5:'c'
                ,6:'k'
                }

    experiments      = []              # which output files to read
    for val in sys.argv[1:]:
      experiments.append(int(val))

    experiment_names = []              # names form XML of each experiment
   
    plot_points      = []              # data, one row per experiment

    exp_map = {}                       # mapping from experiment to row in plot_points

    experiment_start = []
        
    ###################################
    # load experiments
    ###################################
    for experiment in experiments:
        exp_map[experiment] = len(exp_map); 
        plot_points.append([])

        result_file = open("r"+str(experiment)+".xml",'r')
        result      = xml.dom.minidom.parse(result_file) 
        result_file.close()

        experiment_names.append(result.getElementsByTagName('data_structure')[0].childNodes[0].nodeValue)
        experiment_start.append(float(result.getElementsByTagName('startup_time')[0].childNodes[0].nodeValue))

        numbers   = result.getElementsByTagName('number')
        p_looks   = result.getElementsByTagName('present_lookups')
        a_looks   = result.getElementsByTagName('absent_lookups')
        p_seconds = result.getElementsByTagName('present_seconds')
        a_seconds = result.getElementsByTagName('absent_seconds')

        for (number,p_look,a_look,p_seconds,a_seconds) in zip(numbers,p_looks,a_looks,p_seconds,a_seconds):
          plot_points[exp_map[experiment]].append([ int(   number.childNodes[0].nodeValue),
                                                    int(   p_look.childNodes[0].nodeValue), 
                                                    int(   a_look.childNodes[0].nodeValue), 
                                                  float(p_seconds.childNodes[0].nodeValue), 
                                                  float(a_seconds.childNodes[0].nodeValue)])


    ###################################
    # plot data
    ###################################
    plt.figure(1)

    legend_list = []

    for experiment in experiments:
      legend_list.append("+ "+experiment_names[exp_map[experiment]])
      results = [[0,0,0],[0,0,0],[experiment_start[exp_map[experiment]],experiment_start[exp_map[experiment]],experiment_start[exp_map[experiment]]],[0,0,0],[0,0,0],[0,0,0]]
      for [n,p_l,a_l,p_s,a_s] in plot_points[exp_map[experiment]]:
        
         results[0].append(n*100)
         if p_s == 0:
           results[1].append(0)
         else:
           results[1].append(p_l/float(p_s))
         results[2].append(results[2][-1]+p_s)
         results[3].append(results[3][-1]+p_l)
         #results[4].append(results[3][-1]/results[2][-1])
         if (results[2][-1]-results[2][-4]) == 0:
           results[5].append(0.0)
         else:
           results[5].append((results[3][-1]-results[3][-4])/(results[2][-1]-results[2][-4]))
      plt.subplot(521);plt.plot(results[3],results[2],color_map[exp_map[experiment]],label='+ '+experiment_names[exp_map[experiment]])
      plt.subplot(522);plt.plot(results[3],results[5],color_map[exp_map[experiment]])
      plt.subplot(523);plt.plot(results[3],results[2],color_map[exp_map[experiment]])
      plt.subplot(524);plt.plot(results[3],results[5],color_map[exp_map[experiment]])
      plt.subplot(525);plt.plot(results[3],results[2],color_map[exp_map[experiment]])
      plt.subplot(526);plt.plot(results[3],results[5],color_map[exp_map[experiment]])
      plt.subplot(527);plt.plot(results[3],results[2],color_map[exp_map[experiment]])
      plt.subplot(528);plt.plot(results[3],results[5],color_map[exp_map[experiment]])

    for experiment in experiments:
      legend_list.append("- "+experiment_names[exp_map[experiment]])
      #results = [[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0],[0,0,0]]
      results = [[0,0,0],[0,0,0],[experiment_start[exp_map[experiment]],experiment_start[exp_map[experiment]],experiment_start[exp_map[experiment]]],[0,0,0],[0,0,0],[0,0,0]]
      for [n,p_l,a_l,p_s,a_s] in plot_points[exp_map[experiment]]:
        
         results[0].append(n*100)
         if a_s == 0:
           results[1].append(0)
         else:
           results[1].append(a_l/float(a_s))
         results[2].append(results[2][-1]+a_s)
         results[3].append(results[3][-1]+a_l)
         #results[4].append(results[3][-1]/results[2][-1])
         if (results[2][-1]-results[2][-4]) == 0:
           results[5].append(0.0)
         else:
           results[5].append((results[3][-1]-results[3][-4])/(results[2][-1]-results[2][-4]))
      plt.subplot(521);plt.plot(results[3],results[2],color_map[exp_map[experiment]]+'--',label='- '+experiment_names[exp_map[experiment]])
      plt.subplot(522);plt.plot(results[3],results[5],color_map[exp_map[experiment]]+'--')
      plt.subplot(523);plt.plot(results[3],results[2],color_map[exp_map[experiment]]+'--')
      plt.subplot(524);plt.plot(results[3],results[5],color_map[exp_map[experiment]]+'--')
      plt.subplot(525);plt.plot(results[3],results[2],color_map[exp_map[experiment]]+'--')
      plt.subplot(526);plt.plot(results[3],results[5],color_map[exp_map[experiment]]+'--')
      plt.subplot(527);plt.plot(results[3],results[2],color_map[exp_map[experiment]]+'--')
      plt.subplot(528);plt.plot(results[3],results[5],color_map[exp_map[experiment]]+'--')

    plt.subplot(521); 
    plt.ylabel("Seconds Elapsed")
    plt.title("Completed Transactions vs. Seconds Elapsed")
    #plt.legend(loc='lower right', shadow=True, fancybox=True, ncol=2 )
    plt.ticklabel_format(style="plain")

    ax = plt.subplot(522); 
    ax.yaxis.set_label_position('right')
    plt.ylabel("Transactions Per Second")
    plt.title("Completed Transactions vs. Transactions Per Second")
    plt.ticklabel_format(style="plain")

    plt.subplot(523); 
    plt.ylabel("Seconds Elapsed")
    plt.ylim(0,100); plt.xlim(0,15000000)
    plt.ticklabel_format(style="plain")

    ax = plt.subplot(524); 
    ax.yaxis.set_label_position('right')
    plt.ylabel("Transactions Per Second")
    plt.xlim(0,15000000)
    plt.ticklabel_format(style="plain")

    plt.subplot(525); 
    plt.ylabel("Seconds Elapsed")
    plt.ylim(0,80); plt.xlim(0,50000)

    ax = plt.subplot(526); 
    ax.yaxis.set_label_position('right')
    plt.ylabel("Transactions Per Second")
    plt.xlim(0,50000)
    plt.ticklabel_format(style="plain")

    plt.subplot(527); 
    plt.xlabel("Transactions Completed")
    plt.ylabel("Seconds Elapsed")
    plt.ylim(0,1); plt.xlim(0,100)

    ax = plt.subplot(528); 
    ax.yaxis.set_label_position('right')
    plt.ylabel("Transactions Per Second")
    plt.xlabel("Transactions Completed")
    plt.ylim(0,400); plt.xlim(0,100)
    plt.ticklabel_format(style="plain")


    # setup plot
    handles,labels = plt.subplot(521).get_legend_handles_labels()

    ax = plt.subplot(515)
    ax.axis('off')
    ax.legend(handles,labels, loc='lower center', shadow=True, fancybox=True, ncol=2 )


    #divider = make_axes_locatable(ax)
    #legend_ax = divider.append_axes('bottom','10%')
    #legend_ax.axis('off')
    #legend_ax.legend(handles,labels, loc='lower center', shadow=True, fancybox=True, ncol=2 )
    #legend_ax.legend(handles,labels, shadow=True, fancybox=True, ncol=2 )
    #plt.title('Legend')

    plt.subplots_adjust(hspace=0.5, wspace=0.1, bottom=0.05,top=0.95,left=0.05, right = 0.95)
    plt.gcf().set_size_inches(20,16)
    plt.gcf().savefig('plot_interprocess.pdf',bbox_inches='tight',pad_inches=0.1)
    plt.show()

