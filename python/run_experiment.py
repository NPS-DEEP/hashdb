#!/usr/bin/python

"""
This script runs an automated experiment for database load and query functions and plots the information in a graph.

Arguments:
- dbs: a list of databases to test
- debug: set the debug flag
- querytime: the total amount of time that the db should be queried
- loadtime: the total amount of time alloted for loading the db

The following graphs are created from this script.
- Load time vs. number of rows in database
- Query frequency vs. number of rows in database
"""
if __name__=="__main__":
    import argparse 
    import sys,time
    import experiment
    import matplotlib.pyplot as plt
    import matplotlib.mlab as mlab
    
    plot_points={}
    color_map={experiment.DEBUG:'black', 
               experiment.SQLITE:'red', 
               experiment.POSTGRESQL:'yellow', 
               experiment.INNODB:'green', 
               experiment.MYISAM:'blue'}
    
    load_time_index=0
    np_query_time_index=1
    p_query_time_index=2
    
    #DB_SIZES = [1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000]
    DB_SIZES = [10, 100]
    parser = argparse.ArgumentParser(description="DB Benchmark Program")
    parser.add_argument("--dbs",help="Specifies which database(s) to use", type=str, metavar='D', nargs='+', default='all')
    parser.add_argument("--debug",help="Enable debugging",action="store_true")
    parser.add_argument("--querytime",help="Specifies number of seconds to query for",type=int,default=2)
    parser.add_argument("--loadtime",help="Specifies number of seconds to load each database",type=int,default=2)
    args = parser.parse_args()
    
    #print ("ARGUMENTS: dbs = {}, debug = {}, querytime = {}, loadtime = {}".format(args.dbs, args.debug, args.querytime, args.loadtime))
    
    if args.dbs == 'all':
        args.dbs = (experiment.DEBUG,
                    experiment.SQLITE,
                    experiment.POSTGRESQL,
                    experiment.INNODB,
                    experiment.MYISAM)
        
    #output_writer = csv.writer(open("graph_points.csv", "ta"), delimiter='\t')
        
    for db_str in args.dbs:
        # initialize plot points
        plot_points[db_str]=[[],[],[]]
        for curr_row_size in DB_SIZES:
            print ("Running experiment with {} db and {} rows".format(db_str, curr_row_size))
            
            d = experiment.db(argparse.Namespace(db=db_str, debug=args.debug, querytime=args.querytime, rows=curr_row_size))
            #sys.exit(0)
            
            # create database
            d.create()
            
            # run load cycle
            load_start, load_count, load_end = experiment.timed_load(d, args.loadtime, curr_row_size)
            
            # if we weren't able to load all of the rows in 
            # the time period, exit
            if load_count != curr_row_size:
                print ("Took longer than {} seconds to load {} rows into DB!".format(args.loadtime, curr_row_size))
                d.destroy()
                break
            
            # run query cyle
            np_query_start, np_query_count, np_query_end = experiment.timed_query(d, (args.querytime/2), curr_row_size, False)

            p_query_start, p_query_count, p_query_end = experiment.timed_query(d, (args.querytime/2), curr_row_size, True)
            
            # destroy db
            d.destroy()
            plot_points[db_str][load_time_index].append(load_end - load_start)
            plot_points[db_str][np_query_time_index].append( np_query_count / (np_query_end - np_query_start))
            plot_points[db_str][p_query_time_index].append(p_query_count / (p_query_end - p_query_start))

        print ("{} plot points load: {}, np_query_rate: {}, p_query_rate: {}".format(db_str, plot_points[db_str][load_time_index], plot_points[db_str][np_query_time_index], plot_points[db_str][p_query_time_index]))

        '''
        print ("{} {} {} {} {}".format(db_str, 
        curr_row_size, 
        (load_end - load_start),
        np_query_count / (np_query_end - np_query_start),
        p_query_count / (p_query_end - p_query_start)))                            
        
        
        # get the x and y coordinates
        x,y = np.loadtxt(our_file,delimiter='\t', usecols=(2,8), unpack=True)
        # setup plot
        ax = fig.add_subplot(111)
        ax.plot(x, y, 'o-')
        
        plt.xlabel('Time since UNIX epoch')
        plt.ylabel('CWND in bytes')
        plt.title('CWND vs time from file ' + sys.argv[1])
        
        plt.show
        
        output_writer = csv.writer(open("output.csv","ta"),delimiter='\t')
        output_writer.writerow([hostname,args.db,args.querytime,args.rows,
        st,load_start,load_end-load_start,
        np_query_count,np_query_start,np_query_end-np_query_start, 
        p_query_count,p_query_start,p_query_end-p_query_start])
        
        open(ofn+".t1","wt").write(ps_output())
        '''
        
