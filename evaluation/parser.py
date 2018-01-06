import csv
import matplotlib.pyplot as plt

from collections import defaultdict
data = defaultdict(list)

with open('loglistener_random_battery.txt', 'r') as f:
    reader = csv.reader(f,delimiter=' ')
    for row in reader:
        if row[2] is 'P':
            print(row[3], row[6])
            #print(type(int(row[6])))
            data[row[3]].append(int(row[6])/100000)

for i in data:
    print(data[i])
    plt.plot(data[i])
plt.show()