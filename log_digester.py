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
send_through = collections.OrderedDict()
recv_through = collections.OrderedDict()

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

    #find actual start and end events for this connection
    if ts > connections[fd]['end']:
        connections[fd]['end'] = ts
    if ts < connections[fd]['start']:
        connections[fd]['start'] = ts

    #log total events
    connections[fd]['total'] += 1

    #log data amounts
    if fields[2] == 's':
        sn = int(fields[3])
        connections[fd]['sent'] += sn
        if round(ts/1000) in send_through:
            send_through[round(ts/1000)] += sn
        else:
            send_through[round(ts/1000)] = sn
    if fields[2] == 'r':
        rn = int(fields[3])
        connections[fd]['recv'] += rn
        if round(ts/1000) in recv_through:
            recv_through[round(ts/1000)] += rn
        else:
            recv_through[round(ts/1000)] = rn


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
round(packets/total_conn)
print('==>totals<==\nconnections: {}\nsent(bytes): {}\nrecv(bytes): {}'.format(total_conn, sent, recv))
print('==>averages<==\ndelay(ms): {}\npackets sent: {}\ndata sent(bytes): {}\ndata recv(bytes): {}\n'.format(
    round(delay/total_conn), round(packets/total_conn), round(sent/total_conn),round(recv/total_conn)))

plt.subplot(2, 1, 1)
plt.plot(send_through.keys(), send_through.values())
plt.xlabel('Time (ms)')
plt.ylabel('Data (bytes)')
plt.title('Average Sent Bytes')
plt.grid(True)

plt.subplot(2, 1, 2)
plt.plot(recv_through.keys(), recv_through.values())
plt.xlabel('Time (ms)')
plt.ylabel('Data (bytes)')
plt.title('Average Recv Bytes')
plt.grid(True)

plt.show()
