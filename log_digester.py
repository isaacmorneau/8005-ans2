import sys
import matplotlib.pyplot as plt
import numpy as np
import collections

if len(sys.argv) != 2:
    print('usage is {} <log file>'.format(sys.argv[0]))
    sys.exit(0)

logfile = open(sys.argv[1], 'r')
lines = logfile.read().splitlines()

connections = {}
smooshed = {}

current_line= 0
total_lines = len(lines)
last_percent = 0.0

for line in lines:
    #print the progress every percent
    complete = round((current_line / total_lines) * 100)
    current_line += 1
    if complete > last_percent:
        last_percent = complete
        print('{}% [{}/{}]'.format(complete, current_line, total_lines))

    fields = line.split(' ')
    if len(fields) < 3 or fields[2] in ['r', 's'] and len(fields) < 4:
        break
    ts = int(fields[0])
    fd = int(fields[1])

    #initialize new connections
    if fields[2] == 'o':
        connections[fd] = {
            'total': 1,
            'delay': 0,
            'sent': 0,
            'recv': 0,
            'start': ts,
            'end': ts
            }
        continue

    #initialize smooshed data
    smoosh_pos = round(ts/1000)
    if smoosh_pos not in smooshed:
        smooshed[smoosh_pos] = {
                'sent':0,
                'recv': 0,
                'delay':0,
                'fd': {}
                }
    if fd not in smooshed[smoosh_pos]['fd']:
        smooshed[smoosh_pos]['fd'][fd] = {
                'start':ts,
                'end':ts,
                'total':0
                }

    #find actual start and end events for this connection
    if ts > connections[fd]['end']:
        connections[fd]['end'] = ts
    if ts < connections[fd]['start']:
        connections[fd]['start'] = ts


    #store timing for each connection
    smooshed[smoosh_pos]['fd'][fd]['total'] += 1;
    if ts > smooshed[smoosh_pos]['fd'][fd]['end']:
        smooshed[smoosh_pos]['fd'][fd]['end'] = ts;
    if ts < smooshed[smoosh_pos]['fd'][fd]['start']:
        smooshed[smoosh_pos]['fd'][fd]['start'] = ts;

    #log total events
    connections[fd]['total'] += 1

    #log data amounts
    if fields[2] == 's':
        sn = int(fields[3])
        connections[fd]['sent'] += sn
        smooshed[smoosh_pos]['sent'] += sn;
    if fields[2] == 'r':
        rn = int(fields[3])
        connections[fd]['recv'] += rn
        smooshed[smoosh_pos]['recv'] += rn;

for smoosh in smooshed:
    for fd in smooshed[smoosh]['fd']:
        smooshed[smoosh]['delay'] += (smooshed[smoosh]['fd'][fd]['end'] - smooshed[smoosh]['fd'][fd]['start']) / smooshed[smoosh]['fd'][fd]['total']
    smooshed[smoosh]['delay'] /= len(smooshed[smoosh]['fd'])

sent = 0
recv = 0
delay = 0
total_conn = len(connections)
packets = 0
for con in connections:
    connections[con]['delay'] = round((connections[con]['end']-connections[con]['start']) / connections[con]['total'])
    delay += connections[con]['delay']
    sent += connections[con]['sent']
    recv += connections[con]['recv']
    packets += connections[con]['total']
    # thsi just spams out the thousands of records and is not actually useful
    #print('{} :: {}'.format(con,connections[con]))
def totals():
    print('==>totals<==\nconnections: {}\nsent(bytes): {}\nrecv(bytes): {}'.format(total_conn, sent, recv))

def averages():
    print('==>averages<==\ndelay(ms): {}\npackets sent: {}\ndata sent(bytes): {}\ndata recv(bytes): {}\n'.format(round(delay/total_conn), round(packets/total_conn), round(sent/total_conn),round(recv/total_conn)))

sendd = {}
recvd = {}
timed = {}
for sm in smooshed:
    sendd[sm] = smooshed[sm]['sent']
    recvd[sm] = smooshed[sm]['recv']
    timed[sm] = smooshed[sm]['delay']

stk = collections.OrderedDict(sorted(sendd.items(), key=lambda t: t[0]))
rtk = collections.OrderedDict(sorted(recvd.items(), key=lambda t: t[0]))
ttk = collections.OrderedDict(sorted(timed.items(), key=lambda t: t[0]))

def sendrecv_graphs():
    plt.subplot(2, 1, 1)
    plt.plot(stk.keys(), stk.values())
    plt.xlabel('Time (ms)')
    plt.ylabel('Data (bytes)')
    plt.title('Average Sent Bytes')
    plt.grid(True)

    plt.subplot(2, 1, 2)
    plt.plot(rtk.keys(), rtk.values())
    plt.xlabel('Time (ms)')
    plt.ylabel('Data (bytes)')
    plt.title('Average Recv Bytes')
    plt.grid(True)

    plt.show()

def timing_graphs():
    plt.plot(ttk.keys(), ttk.values())
    plt.xlabel('Time (ms)')
    plt.ylabel('Response (ms)')
    plt.title('Average Connection Delay')
    plt.grid(True)

    plt.show()


print("finished parsing")
while True:
    print("==>menu<==")
    print("1 show totals")
    print("2 show averages")
    print("3 show send and recv graphs")
    print("4 show timing graphs")
    print("5 exit")

    choice = input(": ")
    if choice == "1":
        totals()
    elif choice == "2":
        averages()
    elif choice == "3":
        sendrecv_graphs()
    elif choice == "4":
        timing_graphs()
    elif choice == "5":
        break

