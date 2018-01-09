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
def read_file(file_name):
    data = defaultdict(list)
    with open(file_name, 'r') as f:
        reader = csv.reader(f,delimiter=' ')
        for row in reader:
            if (len(row) > 2):
                if row[2] is 'P':
                    data[row[3]].append(int(row[11]))
    return data

def calc_average_consumption(data):
    a = np.array([])
    for i in data:
        a = np.append(a, data[i][-1], data[i][0])
    average = np.average(a)
    print(average)
    return average


unicast_data = read_file('loglistener_random_multihop.txt')
multihop_data = read_file('loglistener_random_unicast.txt')
unicast_average = calc_average_consumption(unicast_data)
multihop_average = calc_average_consumption(multihop_data)

# plot
left = [0, 1]
label = ["unicast", "multihop"]
height = np.array([unicast_average, multihop_average])
plt.bar(left, height, tick_label=label, align="center")
plt.show()