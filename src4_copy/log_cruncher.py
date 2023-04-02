import subprocess
import re
#s1=subprocess.run(['./main.bin'], stdout=subprocess.PIPE).stdout.decode('utf-8')
s1=open("1.txt").read()

print(s1)
print("---------")
s=s1.split("\n")



filters03={
    "child spawned":[r"^CHILD \d+: spawned$"],
    "thread spawned":[r"^THREAD \d+-\d+: Thread spawned$"],
    "thread exit":[r"^THREAD \d+-\d+: someone found result, breaking$",r"^THREAD \d+-\d+: I found a result, but someone beat me to it$",r"^THREAD \d+-\d+: Found valid nonce \d+$","^end of loop$"],
    "child task end":[r"^CHILD \d+: Some other process has a result.$",r"^CHILD \d+: I found a nonce, but someone beat me to it in writing it to the shared memory.$",r"^CHILD \d+: found a valid nonce \d+$"],
    "child exit":[r"^CHILD \d+: No more job flag raised, breaking out of main loop$"]
}

def count(s, f):
    r={}
    for i in f:
        r[i]=0
    for line in s:
        for situation in f:
            for filt in f[situation]: #filter is a keyword
                if re.match(filt, line):
                    r[situation]+=1
                    break
    for i in r:
        print(i, r[i])

count(s, filters03)