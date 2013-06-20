#!/usr/bin/python

"""
    This script runs an automated experiment for database load and query functions and records the information in a file.  It takes the following arguments.

    Arguments:
    - number of rows for the database
    - number of queries 
    - output file name

    The following measurements are recorded from the experiment.

    Measurements:
    - system time
    - process listing
    - sytem load (avg. # of processes in CPU queue)
    - mem usage (total, free, used and cache memory)
    - process running time (for load and query functions)
"""

import sys,os
#sys.path.append(os.getenv("DOMEX_HOME") + "/src/lib/") # add the library
sys.path.append("/home/kmfoster/domex/src/lib") # add the library
sys.path.append("../lib/")                             # add the library

import ttable                   # for making latex tabes

# import statements
import random
import timeit
import time
from time import sleep
import platform
#import statgrab
from subprocess import Popen, PIPE 
import hashlib
import csv
import sqlite3
import psycopg2
import pymysql
import pymongo

class plot:
    import matplotlib.pyplot as plt
    import numpy as np

    RATE_UNIT = "Transactions/Second"
    CURRENT_RATE_LABEL = "Interval Rate ({})".format(RATE_UNIT)
    CUMULATIVE_RATE_LABEL = "Cumulative Rate ({})".format(RATE_UNIT)

    TIME_UNIT = "Seconds"
    TIME_LABEL = "Time ({})".format(TIME_UNIT)

    TRANSACTION_LABEL = "Transactions"

    def __init__(self, data_file):
        import re, sys
        import numpy as np

        self.data_file = data_file
        m = re.search('timed_query_(\w+)-(\w+-?\w+)_(\d+)rows-(\d+)s-(\d+)-(\d+).output', data_file)

        self.mode = m.group(1)
        self.db = m.group(2)
        self.rows = "1Mil" if int(m.group(3)) == 10**6 else "10Mil" if int(m.group(3)) == 10**7 else "100Mil" if int(m.group(3)) == 10**8 else "1Bil"
        self.secs = m.group(4)

        self.title_base = "Transaction Rate of {} queries ({} DB of {} rows for {} seconds)".format(self.mode,
                                                                                               self.db,
                                                                                               self.rows,
                                                                                               self.secs)

        self.pdf_file_base = "TPS_{}queries_{}DB_{}rows_{}seconds".format(self.mode,
                                                                     self.db,
                                                                     self.rows,
                                                                     self.secs)

        self.title_base = "TPS for {} queries on {} DB of {} rows for {} seconds".format(self.mode,
                                                                                       self.db,
                                                                                       self.rows,
                                                                                       self.secs)

        self.transactions, time_delta, self.current_rate = np.loadtxt(self.data_file, delimiter='\t', usecols=(0,1,2), unpack=True)

        self.cumulative_time = np.cumsum(time_delta)
        
        self.cumulative_rate = np.asarray([float(t)/c if c>0 else 0 for t,c in zip(self.transactions, self.cumulative_time)])

        '''
        print("transactions={}\n cumulative_time=={}\n current_rate={}\n cumulative_rate = {}".format(self.transactions,
                                                                                                      self.cumulative_time,
                                                                                                      self.current_rate,
                                                                                                      self.cumulative_rate))
        '''

    def plot_general(self,title, pdf_file, line_label, x_array, x_label, y_array, y_label, axis_pos):
        import matplotlib.pyplot as plt

        fig = plt.figure()
        ax = fig.add_subplot(111)
        ax.plot(x_array,y_array,label=line_label)
        leg = ax.legend((line_label,), axis_pos)
        plt.title(title)
        plt.xlabel(x_label)
        plt.ylabel(y_label)
        plt.savefig(pdf_file)

    def plot_transactions_vs_current_rate(self):
        title = "Current {}".format(self.title_base)

        print("Plotting {}".format(title))

        pdf_file = "{}-TransVSCurrRate.pdf".format(self.pdf_file_base)
        line_label = self.db
        x_array = self.transactions
        x_label = self.TRANSACTION_LABEL
        y_array = self.current_rate
        y_label = self.CURRENT_RATE_LABEL
        axis_pos = 'lower right'
        
        self.plot_general(title, pdf_file, line_label, x_array, x_label, y_array, y_label, axis_pos)


    def plot_transactions_vs_cumulative_rate(self):
        title = "Cumulative {}".format(self.title_base)
        
        print("Plotting {}".format(title))

        pdf_file = "{}-TransVSCumuRate.pdf".format(self.pdf_file_base)
        line_label = self.db
        x_array = self.transactions
        x_label = self.TRANSACTION_LABEL
        y_array = self.cumulative_rate
        y_label = self.CUMULATIVE_RATE_LABEL
        axis_pos = 'lower right'

        self.plot_general(title, pdf_file, line_label, x_array, x_label, y_array, y_label, axis_pos)

    def plot_time_vs_current_rate(self):
        title = "Current {}".format(self.title_base)

        print("Plotting {}".format(title))

        pdf_file = "{}-TimeVSCurrRate.pdf".format(self.pdf_file_base)
        line_label = self.db
        x_array = self.cumulative_time
        x_label = self.TIME_LABEL
        y_array = self.current_rate
        y_label = self.CURRENT_RATE_LABEL
        axis_pos = 'lower right'

        self.plot_general(title, pdf_file, line_label, x_array, x_label, y_array, y_label, axis_pos)


    def plot_time_vs_cumulative_rate(self):
        title = "Cumulative {}".format(self.title_base)

        print("Plotting {}".format(title))

        pdf_file = "{}-TimeVSCumuRate.pdf".format(self.pdf_file_base)
        line_label = self.db
        x_array = self.cumulative_time
        x_label = self.TIME_LABEL
        y_array = self.cumulative_rate
        y_label = self.CUMULATIVE_RATE_LABEL
        axis_pos = 'lower right'

        self.plot_general(title, pdf_file, line_label, x_array, x_label, y_array, y_label, axis_pos)

    def plot_all(self):

        self.plot_transactions_vs_cumulative_rate()
        self.plot_transactions_vs_current_rate()
        self.plot_time_vs_cumulative_rate()
        self.plot_time_vs_current_rate()

    def plot_one_graph(self):
        import matplotlib.pyplot as plt
        
        print("Plotting {}".format(self.title_base))

        fig = plt.figure(figsize=(9,9))
        line_label=self.db
        # transaction vs current_rate
        title = "Transactions vs Interval Rate"
        print("\tPlotting {}".format(title))
        
        x_array = self.transactions/100
        x_label = "Hundreds of {}".format(self.TRANSACTION_LABEL)
        y_array = self.current_rate
        y_label = self.CURRENT_RATE_LABEL
        
        ax = fig.add_subplot(224)
        ax.plot(x_array,y_array,label=line_label)
        ax.set_title(title)
        ax.set_xlabel(x_label, fontsize='small')
        for tick in ax.xaxis.get_major_ticks():
            tick.label.set_fontsize('xx-small')
        ax.set_ylabel(y_label, fontsize='small')
        ax.yaxis.tick_right()

        # plot transactions vs cumulative rate
        #title = "Cumulative {}".format(self.title_base)
        title = "Transactions vs Cumulative Rate"

        print("\tPlotting {}".format(title))
        
        x_array = self.transactions/100
        x_label = "Hundreds of {}".format(self.TRANSACTION_LABEL)
        y_array = self.cumulative_rate
        y_label = self.CUMULATIVE_RATE_LABEL
        
        ax = fig.add_subplot(222)
        ax.plot(x_array,y_array,label=line_label)
        ax.set_title(title)
        ax.set_xlabel(x_label, fontsize='small')
        for tick in ax.xaxis.get_major_ticks():
            tick.label.set_fontsize('xx-small')
        ax.set_ylabel(y_label, fontsize='small')
        ax.yaxis.tick_right()
        
        # plot_time_vs_current_rate
        title = "Time vs Interval Rate"
        print("\tPlotting {}".format(title))
        
        x_array = self.cumulative_time
        x_label = self.TIME_LABEL
        y_array = self.current_rate
        y_label = self.CURRENT_RATE_LABEL
        
        ax = fig.add_subplot(223)
        ax.plot(x_array,y_array,label=line_label)
        ax.set_title(title)
        ax.set_xlabel(x_label, fontsize='small')
        for tick in ax.xaxis.get_major_ticks():
            tick.label.set_fontsize('xx-small')
        ax.set_ylabel(y_label, fontsize='small')
        
        # plot_time_vs_cumulative_rate
        title = "Time vs Cumulative Rate"
        print("\tPlotting {}".format(title))
        
        x_array = self.cumulative_time
        x_label = self.TIME_LABEL
        y_array = self.cumulative_rate
        y_label = self.CUMULATIVE_RATE_LABEL
        
        ax = fig.add_subplot(221)
        ax.plot(x_array,y_array,color='b',label=line_label)
        ax.set_title(title)
        ax.set_xlabel(x_label, fontsize='small')
        for tick in ax.xaxis.get_major_ticks():
            tick.label.set_fontsize('xx-small')
        ax.set_ylabel(y_label, fontsize='small')

        legend = plt.legend((line_label,), 'best')
        plt.setp(legend.get_texts(), fontsize='small')

        plt.figtext(0.5, 0.965,  self.title_base, ha='center', color='black', weight='bold', size='large')
        plt.savefig("{}.pdf".format(self.pdf_file_base))
                    

class db:
    # Supported databases
    SQLITE = "sqlite"
    SQLITE_MEM = "sqlite-mem"
    DEBUG  = "debug"
    POSTGRESQL = "postgresql"
    INNODB = "mysql-innodb"
    MYISAM = "mysql-myisam"
    MONGODB = "mongodb"
    PYTHON = "python"
    STL = "stl"

    supported_databases = [DEBUG,SQLITE,SQLITE_MEM,POSTGRESQL,INNODB,MYISAM, MONGODB, PYTHON, STL]

    def __init__(self, db_string, debug, rows):
        if db_string not in self.supported_databases:
            raise RuntimeError("This program only supports these databases: "+" ".join(self.supported_databases))
        self.db = db_string
        self.debug = debug
        self.rows = rows
        if self.db == self.INNODB:
            self.db_name="db_{}_{}".format("innodb",self.rows)
            self.table_name="hashes_{}_{}".format("innodb", self.rows)
        elif self.db == self.MYISAM:
            self.db_name="db_{}_{}".format("myisam", self.rows)
            self.table_name="hashes_{}_{}".format("myisam", self.rows)
        else:
            self.db_name="db_{}_{}".format(self.db,self.rows)
            self.table_name="hashes_{}_{}".format(self.db, self.rows)
        print("Created db object: {} {} {} {}".format(self.db,
                                                      self.rows,
                                                      self.db_name,
                                                      self.table_name))
        self.start()
        self.connect()
        
    def __del__(self):
        self.disconnect()
        self.stop()

    def disconnect(self):
        if self.db==self.SQLITE or self.db==self.POSTGRESQL or self.db==self.INNODB or self.db==self.MYISAM:
            if hasattr(self, 'curr'): 
                self.cur.close()
            if hasattr(self, 'conn'):
                self.conn.close()
            return
        if self.db==self.MONGODB:
            if hasattr(self, 'conn'):
                self.conn.disconnect()
            return

    def connect(self):
        if self.db==self.DEBUG or self.db==self.PYTHON or self.db==self.STL:
            return
        if self.db==self.SQLITE_MEM:
            self.conn = sqlite3.connect(":memory:")
            self.cur = self.conn.cursor()
            return
        if self.db==self.SQLITE:
            self.conn = sqlite3.connect('/raid/kmfoster/sqlite/{}'.format(self.db_name))
            self.cur = self.conn.cursor()
            # set the cache size -67108864 is translated as 64GB to
            # -15728640 is translated as 15GB
            self.cur.execute('pragma cache_size=-67108864;')
            self.cur.execute('pragma cache_size;')
            print("cache_size = {}".format(self.cur.fetchone()))
            self.cur.execute('pragma page_size;')
            print("page_size = {}".format(self.cur.fetchone()))
            return
        if self.db==self.POSTGRESQL:
            ''' requires that a db named 'test' was previously created 
            and that the authentication type for local connections is trust.'''
            self.conn = psycopg2.connect("dbname={} user=test host=localhost port=7777".format(self.db_name))
            self.cur = self.conn.cursor()
            if self.cur is None:
                print('Yikes, cur is none')
            return
        if self.db==self.INNODB or self.db==self.MYISAM:
            ''' requires that a db and user account 'test' were previously
            created and that the user 'test' has no password '''
            self.conn = pymysql.connect(user='test', db=self.db_name, host='localhost', port=7778)
            self.cur = self.conn.cursor()
            return
        if self.db==self.MONGODB:
            ''' requires that mongodb is running '''
            
            self.conn = pymongo.Connection("localhost", 7779)
            self.d = self.conn[self.db_name]
            self.col = self.d[self.table_name]
        
            return

    def start(self):
        import subprocess

        print ("Starting {}".format(self.db))

        if self.db == self.INNODB or self.db==self.MYISAM:
            print("{} must be started manually!".format(self.db))
            return
        if self.db==self.POSTGRESQL:
            ans = subprocess.getoutput("sudo -u kmfoster /home/kmfoster/domex/src/sectorid/db_ctl.sh start postgresql")
            print(ans)
            return
        if self.db==self.MONGODB:
            ans = subprocess.getoutput("sudo -u kmfoster /home/kmfoster/domex/src/sectorid/db_ctl.sh start mongodb")
            ans=""
            print(ans)
            return 

            
    def stop(self):
        import subprocess

        print ("Stopping {}".format(self.db))
        if self.db==self.POSTGRESQL:
            ans = subprocess.getoutput("sudo -u kmfoster /home/kmfoster/domex/src/sectorid/db_ctl.sh stop postgresql")
            print(ans)
            return
        if self.db==self.MONGODB:
            ans = subprocess.getoutput("sudo -u kmfoster /home/kmfoster/domex/src/sectorid/db_ctl.sh stop mongodb mongod.pid")
            print(ans)
            return
        
    def restart(self):
        if self.db==self.SQLITE_MEM or self.db==self.PYTHON:
            return

        self.disconnect()
        self.stop()
        self.start()
        self.connect()

    def create(self):
        global db
        """Create the database"""
        if self.debug:
            print("Creating the {} table".format(self.table_name))
        if self.db==self.DEBUG or self.db==self.STL:
            return
        if self.db==self.PYTHON:
            self.hashes={}
            return
        if self.db==self.SQLITE or self.db==self.SQLITE_MEM:
            self.cur.execute('drop table if exists {}'.format(self.table_name))
            self.cur.execute('create table {} (hash text, fileid int, offset int)'.format(self.table_name))
            self.commit()
            return
        if self.db==self.POSTGRESQL:
            self.cur.execute('drop table if exists {}'.format(self.table_name))
            self.cur.execute('create table {} (id serial, hash char(32) primary key, fileid bigint, fileoffset integer)'.format(self.table_name))
            self.commit()
            return
        if self.db==self.INNODB:
            self.cur.execute('drop table if exists {}'.format(self.table_name))
            self.cur.execute('create table {} (id bigint, hash char(32) primary key, fileid bigint, fileoffset integer) engine=innodb'.format(self.table_name))
            self.commit()
            return
        if self.db==self.MYISAM:
            self.cur.execute('drop table if exists {}'.format(self.table_name))
            self.cur.execute('create table {} (id bigint, hash char(32) primary key, fileid bigint, fileoffset integer) engine=myisam'.format(self.table_name))
            self.commit()
            return
        if self.db==self.MONGODB:
            self.d.drop_collection(self.table_name)
            self.col = self.d[self.table_name]
            self.commit
            return
        raise RuntimeError("Unsupported database "+self.db)

    
    def store(self,num):
        """Generic function to store the value in the database using our algorithm"""
        m=hashlib.md5()
        m.update("{}\n".format(num).encode('utf-8'))
        hex = m.hexdigest()
        fileid = num
        offset = num & (2**31-1)
        if self.debug:
            print("storing {} {} {} {}".format(num,hex,num,offset))
        if self.db==self.DEBUG or self.db==self.STL:
            return
        if self.db==self.PYTHON:
            self.hashes[hex]=(num, num, offset)
            return
        if self.db==self.SQLITE or self.db==self.SQLITE_MEM:
            self.cur.execute('insert into {} values (?,?,?)'.format(self.table_name), (hex, fileid, offset))
            return
        if self.db==self.POSTGRESQL or self.db==self.INNODB or self.db==self.MYISAM:
            self.cur.execute('insert into {} (hash, fileid, fileoffset) values (%s, %s, %s)'.format(self.table_name), (hex, fileid, offset))
            return
        if self.db==self.MONGODB:
            self.col.save({"hash":hex, "fileid":fileid, "offset":offset})
            return
        raise RuntimeError("Unsupported database "+self.db)

    def commit(self):
        if self.debug:
            print("commiting database")
            return
        if self.db==self.DEBUG or self.db==self.MONGODB or self.db==self.STL or self.db==self.PYTHON:
            return
        if self.db==self.SQLITE or self.db==self.SQLITE_MEM:
            self.conn.commit()
            return
        if self.db==self.SQLITE or self.db==self.POSTGRESQL or self.db==self.INNODB or self.db==self.MYISAM:
            self.conn.commit()
            return
        raise RuntimeError("Unsupported database "+self.db)

    # This is a generic function used to call a db query function.
    def query(self,query,exists):
        """Generic function to query an existing or non-existing value in the database"""  
        m=hashlib.md5()
        if exists:
            m.update("{}\n".format(query).encode('utf-8'))
        else:
            m.update("{}x".format(query).encode('utf-8'))
        hex = m.hexdigest()
        if self.debug: print("querying {} {} {} ".format(query, hex,exists))
        if self.db==self.DEBUG or self.db==self.STL:
            return 0
        if self.db==self.PYTHON:
            start_time=time.time()
            hex in self.hashes
            end_time=time.time()
            return (end_time-start_time)
        if self.db==self.SQLITE or self.db==self.SQLITE_MEM:
            start_time=time.time()
            self.cur.execute('select fileid from {} where hash=?'.format(self.table_name), (hex,))
            #self.conn.commit()
            #print("ans = {}".format(self.cur.fetchone()))
            #self.cur.execute('pragma cache_size;')
            #print("cache_size = {}".format(self.cur.fetchone()))
            end_time=time.time()
            return (end_time-start_time)
        if self.db==self.POSTGRESQL or self.db==self.INNODB or self.db==self.MYISAM :
            start_time=time.time()            
            self.cur.execute('select fileid from {} where hash=%s'.format(self.table_name), (hex,))
            #self.conn.commit()
            end_time=time.time()
            return (end_time-start_time)
        if self.db==self.MONGODB:
            start_time=time.time()
            self.col.find_one({"hash":hex})
            end_time=time.time()
            return (end_time-start_time)
        raise RuntimeError("Unsupported database "+self.db)

    def destroy(self):
        if self.debug: print("Destroying the database")
        if self.db==self.DEBUG or self.db==self.PYTHON or self.db==self.STL:
            return
        if self.db==self.SQLITE or self.db==self.SQLITE_MEM or self.db==self.POSTGRESQL or self.db==self.INNODB or self.db==self.MYISAM:
            self.cur.execute('drop table if exists {}'.format(self.table_name))
            self.conn.commit()
            return
        if self.db==self.MONGODB:
            #self.d.drop_collection(self.table_name)
            self.d.drop_collection(self.table_name)
            return
        raise RuntimeError("Unsupported database "+self.db)

# Functions for learning about what's going on this computer

# This is a function count the number of currently running processes
def ps_output():
    from subprocess import Popen,call,PIPE
    return Popen(['ps','auxww'],stdout=PIPE).communicate()[0].decode('utf-8')

# This function prints out the virtual and resident memory size of the 
# given command
def ps_output_cmd(cmd):
    from subprocess import Popen,call,PIPE
    p1 = Popen(['ps','-C', cmd, '-o', 'rss,vsz'],stdout=PIPE)
    p2 = Popen(['head', '-2'], stdin=p1.stdout, stdout=PIPE)
    return Popen(['tail', '-1'], stdin=p2.stdout, stdout=PIPE).communicate()[0].decode('utf-8')
    #return Popen(['gre],stdout=PIPE, stdin=p1.stdout).communicate()[0].decode('utf-8')

def sync():
    from subprocess import Popen,call,PIPE
    ans = Popen(['sync'],stdout=PIPE).communicate()[0].decode('utf-8')
    print(ans)
    return ans
    #return Popen(['sync'],stdout=PIPE).communicate()[0].decode('utf-8')

def drop_caches():
    import subprocess
    ans = subprocess.getoutput('echo 3 > /proc/sys/vm/drop_caches')
    print(ans)
    return ans
    
def set_shmmax_high():
    from subprocess import Popen, PIPE
    p = Popen(['sysctl',
               '-w',
               'kernel.shmmax=37580963840'],
                stdout=PIPE,
                stderr=PIPE)
    print(p.communicate()[0])
    return

def set_shmmax_default():
    from subprocess import Popen, PIPE
    p = Popen(['sysctl',
               '-w',
               'kernel.shmmax=33554432'],
                stdout=PIPE,
                stderr=PIPE)
    print(p.communicate()[0])
    return

'''
This function queries a database for a set amount of time and return the 
query start and end time as well as the number of queries that were completed.


Arguments:
 - d: an instance of the db class
 - query_time: the number of seconds that the db should be queried
 - rows: the number of rows in the db
 - is_present: a flag that indicates if the queries are for present or non-present
  entries in the db.
'''
def timed_query(d, query_time, rows, is_present):
    if is_present:
        print("Querying for hashes that ARE present in {} for {} second(s)".format(d.db,query_time))
    else:
        print("Querying for hashes that ARE NOT present in {} for {} second(s)".format(d.db,query_time))
    
    if d.db==db.STL:
        return

    random.seed(0)

    query_start = time.time()
    query_count = 0
    query_end   = time.time()
    
    elapsed_time = query_end-query_start;
    total_query_time = 0
    last_total_query_time = total_query_time

    import datetime
    now = datetime.datetime.now()

    query_file = "timed_query_{}-{}_{}rows-{}s-{}{:02}{:02}-{:02}{:02}{:02}.output".format(
        "present" if is_present else "absent",
        d.db,
        rows,
        query_time,
        now.year,
        now.month,
        now.day,
        now.hour,
        now.minute,
        now.second)

    output_writer = csv.writer(open(query_file,"ta"), delimiter='\t')

    # query for hashes that are not present
    #while query_end - query_start < query_time:
    while elapsed_time < query_time:
        if query_count % 1000 == 0:
            # the amount of time since the last 1,000 queries completed
            query_time_delta = total_query_time - last_total_query_time
            
            # the number of queries per second that have been completed since
            # the last 1,000 queries
            curr_query_rate = 1000/query_time_delta if query_time_delta > 0 else 0 
            print("Queried {} ({} seconds to go)...".format(query_count, (query_time - elapsed_time)))

            # get resident and virutal memory usage from ps output command
            # only available for mysql, postgres and mongo
            # not cmd available to determine cache use of sqlite
            mem_usage = ('N/A', 'N/A')
            if d.db == db.MYISAM or d.db == db.INNODB:
                mem_usage = ps_output_cmd('mysqld').split()
            elif d.db == db.POSTGRESQL:
                mem_usage = ps_output_cmd('postgres').split()
            elif d.db == db.MONGODB:
                mem_usage = ps_output_cmd('mongod').split()
                
            output_writer.writerow([query_count,
                                    #round(total_query_time, 2),
                                    round(query_time_delta, 2),
                                    round(curr_query_rate, 2), 
                                    mem_usage[0],
                                    mem_usage[1]])
            
            # keep track of the current query time for the 
            # next 1,000 queries
            last_total_query_time = total_query_time

        # Perform 10 queries at once before we check our time
        # this granularity is necessary for the larger DBs that 
        # take more time to complete queries
        for i in range(0,10):
            #last_total_query_time = total_query_time
            total_query_time += d.query(int(random.random()*rows), is_present)
            query_count += 1

        # See if db.querytime minutes have elapsed
        query_end = time.time()
        elapsed_time = query_end - query_start;

    #return (query_start, query_count, query_end)
    return (0, query_count, total_query_time, query_file)

'''
This function loads a database with a given number of rows for a specified
amount of time.  It returns the load start and end time as well as the number
of rows that were loaded within the specified time limit.  


Arguments:
 - d: an instance of the db class
 - load_upper_limit: the number of seconds we can load rows before exiting the 
 function.  If this value is -1 then there is no upper limit on the length
 of time to load the rows.
 - rows: the number of rows we should attempt to load in the db
'''
def timed_load(d, load_upper_limit, rows):
    print("Loading {} rows, load upper time limit is {} second(s)".format(rows, load_upper_limit))

    if d.db==db.STL:
        return

    load_start = time.time()
    load_end = time.time()

    for i in range(0,rows):
        d.store(i)

        # print out status every 10,000 loaded
        if i % 100000 == 0:
            d.commit()
            load_end = time.time()
            print("Loaded {} rows in {} seconds.".format(i, (load_end - load_start)))

        # check every 100 if we've exceeded the upper limit for loading
        if (load_upper_limit > 0) and i % 100 == 0:
            load_end = time.time()
            if (load_end - load_start) >= load_upper_limit:
                break
       
    d.commit()
    load_end = time.time()

    return (load_start, (i+1), load_end)



    
if __name__=="__main__":
    import argparse 
    import sys,time

    MODE_BUILD='build'
    MODE_QUERY_ABS='query_abs'
    MODE_QUERY_PRES='query_pres'
    MODE_ALL='all'
    supported_modes=[MODE_BUILD, MODE_QUERY_ABS, MODE_QUERY_PRES, MODE_ALL]

    parser = argparse.ArgumentParser(description="DB Benchmark Program")
    parser.add_argument("--mode", help="Specifies the mode of the experiment {}".format(supported_modes), type=str, metavar='M', nargs=1)
    parser.add_argument("--dbs",help="Specifies which database(s) to use", type=str, metavar='D', nargs='+', default='all')
    parser.add_argument("--rows",help="Specifies list of database sizes to use", type=int, nargs='+', default=-1)
    parser.add_argument("--debug",help="Enable debugging",action="store_true")
    parser.add_argument("--querytime",help="Specifies number of seconds to query for",type=int,default=2)
    parser.add_argument("--loadtime",help="Specifies number of seconds to load each database",type=int,default=-1)
    args = parser.parse_args()
 
    if args.dbs == ['all'] or args.dbs == 'all':
        args.dbs = " ".join(db.supported_databases)

    if args.rows == -1:
        args.rows = (1000,
                     10000,
                     100000,
                     1000000)
                     
    if args.mode is None or args.mode[0] not in supported_modes:
        print("mode {} is not supported, exiting!".format(args.mode))
        sys.exit() 

    mode=args.mode[0]
        
    hostname = platform.node() # hostname, can also use platform.uname()[1] for more detailed info

    for db_str in args.dbs:

        for curr_row_size in args.rows:

            d = db(db_str, args.debug,curr_row_size)
            output_writer = csv.writer(open("output.csv","ta"),delimiter='\t')
                            
            ''' BUILDING DB '''
            if mode == MODE_BUILD or mode == MODE_ALL:
                print ("")
                print ("Building {} db with {} rows".format(db_str, curr_row_size))

                # create database
                d.create()

                # run load cycle
                load_start, load_count, load_end = timed_load(d, args.loadtime, curr_row_size)

                # write load time to output file
                output_writer.writerow([hostname,
                                        "{}_{}".format(db_str,curr_row_size), 
                                        MODE_BUILD,
                                        load_count,
                                        (load_end-load_start)])
            
                # if going to query absent next, then restart the db to clear internal cache
                if mode == MODE_ALL:
                    d.restart()

            if mode == MODE_QUERY_ABS or mode == MODE_ALL:
                print ("Querying for absent entries in {} db with {} rows for {} seconds.".format(db_str, 
                                                                                                  curr_row_size,
                                                                                                  args.querytime))

                # run query cyle
                np_query_start, np_query_count, np_query_end, query_file = timed_query(d, args.querytime, curr_row_size, False)
                
                # write query time to output file
                output_writer.writerow([hostname, 
                                        "{}_{}".format(db_str,curr_row_size), 
                                        MODE_QUERY_ABS, 
                                        args.querytime, 
                                        np_query_count,
                                        (np_query_end-np_query_start)])

                # plot the query rate
                curr_plot = plot(query_file)
                curr_plot.plot_all()
                curr_plot.plot_one_graph()
 
                # if going to query present next, then restart the db to clear internal cache
                if mode == MODE_ALL:
                    d.restart()

            if mode == MODE_QUERY_PRES or mode == MODE_ALL:
                print ("Querying for present entries in {} db with {} rows for {} seconds.".format(db_str, 
                                                                                                   curr_row_size,
                                                                                                   args.querytime))
                # run query cycle
                p_query_start, p_query_count, p_query_end, query_file = timed_query(d, args.querytime, curr_row_size, True)

                # write query time to output file
                output_writer.writerow([hostname,
                                        "{}_{}".format(db_str,curr_row_size), 
                                        MODE_QUERY_PRES,
                                        args.querytime, 
                                        p_query_count,
                                        (p_query_end-p_query_start)])

                # plot the query rate
                curr_plot = plot(query_file)
                curr_plot.plot_all()      
                curr_plot.plot_one_graph()

    print ("Done!")
 


