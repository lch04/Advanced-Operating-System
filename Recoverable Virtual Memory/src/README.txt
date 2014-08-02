Q: How you use logfiles to accomplish persistency plus transaction semantics.
A: When the commit function is called, the in-memory data of the segment(s) being mapped
   by the current transaction are written as structured data into the log file. Since the 
   log file is permanently persisted on disk, data persistency is accomplished.

Q: What goes in them? How do the files get cleaned up, so that they do not expand indefinitely?
A: The log files are structured as follows: 
   *Each entry in the log file corresponds to an external segment.
   *The first line in each entry is a string: "BOS" to mark the begin of the log entry.
   *Following the beginning mark is segment name and data size in bytes, each 
    occupying one separate line.  
   *Starting from next line is the actual data.
   *Following the data is a string: "EOS" to mark the end of the log entry.
   *After ending mark is a separate empty line.
   
   To clean up the entries in the log file, i.e. when rvm_truncate_log function is 
   called, then traverse all the existing entries, and 
   compare the entries' segment names. Suppose there are two entries in the log file 
   that have the same segment name, then we compare the data size of the two entries: if the 
   latter one is equal or bigger than the former one, then simply replace the former by the latter
   because we are only interested in the lastest changes; if the latter one is smaller,
   let's say its size is n, then overwrite the first n bytes of data in the former entry, 
   and remove the the latter entry.  

   This way, we are able to successfully shrink the size of the log file.
