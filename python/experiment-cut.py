# check for correct number of arguments
if (len(sys.argv) != 4):
    print "Usage: " + sys.argv[0] + " <num rows> <num queries> <output file>"
    sys.exit(0)

db_imp = "None"

# get start measurements

'''
    http://www.i-scream.org/libstatgrab/docs/sg_get_load_stats.3.html

    statgrab.sg_get_load_stats() is a wrapper to the getloadavg system call,
    which returns the number of processes in the system run queue averaged
    over various periods of time.  sg_get_load_stats() returns the average over
    1, 5 and 15 minutes.
'''
start_load = statgrab.sg_get_load_stats()

'''
    http://www.i-scream.org/libstatgrab/docs/sg_get_mem_stats.3.html

    statgrab.sg_get_mem_stats() returns the total amount of memory, free
    memory, used memory and cache memory in bytes.
    
'''
start_mem_stats = statgrab.sg_get_mem_stats()

start_num_proc = numproc()


# Load the database
t = timeit.Timer(stmt="""
for x in xrange(num_rows):
    

    offset = x % (2**32)

    dbload(row_id, hash_val, file_id, offset)
""", setup="from __main__ import num_rows; import hashlib; from __main__ import dbload")

load_time = t.timeit(1)

# Query for H_out
t = timeit.Timer(stmt="""
for samp in random.sample(xrange(num_rows), num_queries):
    m=hashlib.md5()
    m.update(str(samp) + 'x')
    h_out=str(m.digest())
    dbquery(h_out)
""", setup="from __main__ import dbquery; from __main__ import num_rows; from __main__ import num_queries; import hashlib; import random; from time import sleep")

query_h_out_time = t.timeit(1)

# Query for H_in
t = timeit.Timer(stmt="""
for samp in random.sample(xrange(num_rows), num_queries):
    m=hashlib.md5()
    m.update(str(samp) + '\\n')
    h_in=str(m.digest())
    dbquery(h_in)
""", setup="from __main__ import dbquery; from __main__ import num_rows; from __main__ import num_queries; import hashlib; import random; from time import sleep")

query_h_in_time = t.timeit(1)

# get end measurements
end_load = statgrab.sg_get_load_stats() 
end_mem_stats = statgrab.sg_get_mem_stats()
end_num_proc = numproc()

# record measurements
f = open(output_file, 'w')


printstr = hostname + "," + \
    db_imp + "," + \
    str(start_time) + "," + \
    str(start_load) + "," + \
    str(start_mem_stats) + "," + \
    str(start_num_proc) + "," + \
    str(end_time) + "," + \
    str(end_load) + "," + \
    str(end_mem_stats) + "," + \
    str(end_num_proc) + "," + \
    str(load_time) + "," + \
    str(query_h_out_time) + "," + \
    str(query_h_in_time) + "\n"

f.write(printstr)

f.close()
