import sys

if len(sys.argv) != 2:
    print('usage is {} <log file>'.format(sys.argv[0]))
    sys.exit(0)

logfile = open(sys.argv[1], 'r')

connections = {}

i = 0
last_percent = 0.0
lines = logfile.read().splitlines()
total = len(lines)
for line in lines:
    complete = round((i / total) * 100)
    i += 1
    if complete > last_percent:
        last_percent = complete
        print('{}% [{}/{}]'.format(complete, i, total))
    fields = line.split(' ')
    ts = int(fields[0])
    fd = int(fields[1])
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
    if ts > connections[fd]['end']:
        connections[fd]['end'] = ts
    if ts < connections[fd]['start']:
        connections[fd]['start'] = ts

    connections[fd]['total'] += 1

    if fields[2] == 's':
        connections[fd]['sent'] += int(fields[3])
    if fields[2] == 'r':
        connections[fd]['recv'] += int(fields[3])


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
    print('{} :: {}'.format(con,connections[con]))
round(packets/total_conn)
print('==>totals<==\nconnections: {}\nsent(bytes): {}\nrecv(bytes): {}'.format(total_conn, sent, recv))
print('==>averages<==\ndelay(ms): {}\npackets sent: {}\ndata sent(bytes): {}\ndata recv(bytes): {}\n'.format(
    round(delay/total_conn), round(packets/total_conn), round(sent/total_conn),round(recv/total_conn)))

