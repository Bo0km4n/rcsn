import csv
import matplotlib.pyplot as plt
import numpy as np
from collections import defaultdict

# for P lines
# 0-> str,
# 1 -> clock_time()
# 2-> P
# 3->rimeaddr_node_addr.u8[0],rimeaddr_node_addr.u8[1], 
# 4-> seqno,
# 5 -> all_cpu,
# 6-> all_lpm,
# 7-> all_transmit,
# 8-> all_listen,
# 9-> all_idle_transmit,
# 10-> all_idle_listen,
# 11->cpu,
# 12-> lpm,
# 13-> transmit,
# 14-> listen, 
# 15 ->idle_transmit, 
# 16 -> idle_listen, [RADIO STATISTICS...]

# consumption jule
# (cycle - before cycle) * 3(voltage) * 0.5 / (32768(RTIMER_SECOND) * 10(margin seconds))
# calculate each cpu, lpm, rx, tx and to sum, prove total consumtion battery(J)
def read_file(file_name):
    data = defaultdict(list)
    with open(file_name, 'r') as f:
        reader = csv.reader(f,delimiter=' ')
        for row in reader:
            if (len(row) > 2):
                obj = {}
                if row[2] is 'P':
                    obj["cpu"] = int(row[5])
                    obj["lpm"] = int(row[6])
                    obj["tx"] = int(row[7])
                    obj["rx"] = int(row[8])
                    data[row[3]].append(obj)
                    
    return data

def calc_average_consumption(data):
    a = []
    for i in data:
        node_comsumption = 0
        for j in range (1, len(data[i])):
            cpu = (data[i][j]["cpu"] - data[i][j-1]["cpu"]) * 3 * 0.5 / 32768 / 10
            lpm = (data[i][j]["lpm"] - data[i][j-1]["lpm"]) * 3 * 0.5 / 32768 / 10
            tx = (data[i][j]["tx"] - data[i][j-1]["tx"]) * 3 * 0.5 / 32768 / 10
            rx = (data[i][j]["rx"] - data[i][j-1]["rx"]) * 3 * 0.5 / 32768 / 10
            node_comsumption += cpu + lpm + tx + rx
        a.append(node_comsumption)
        average = np.average(a)
    return average

unicast_data = read_file('unicast_search_battery.txt')
multihop_data = read_file('multihop_search_battery.txt')
unicast_average = calc_average_consumption(unicast_data)
multihop_average = calc_average_consumption(multihop_data)
print(unicast_average)
print(multihop_average)

# plot
left = [0, 1]
label = ["referrer", "random"]
height = np.array([unicast_average, multihop_average])
plt.bar(left, height, tick_label=label, color="#808080", edgecolor="#000000", align="center")
plt.ylabel("mW")
plt.show()