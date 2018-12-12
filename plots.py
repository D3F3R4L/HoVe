import matplotlib.pyplot as plt
import numpy as np
from math import log10
import glob as gl
#plt.style.use('classic')

def sinr():
    a2a4Files = gl.glob('noop*/DlRsrp*')
    a2a4 = []
    for i in a2a4Files:
        with open(i) as f:
            lines = f.readlines()
            a2a4.append([10 * log10(float(i.split('\t')[5])) for i in lines[1:]])
    a2a4 = np.array(a2a4)
    plt.figure()
    plt.errorbar([float(i.split('\t')[0]) for i in lines[1:]], np.mean(a2a4, axis=0),
                xerr=np.std(a2a4, axis=0), errorevery=100 )
    plt.show()


def main():
    sinr = []
    rsrp = []
    cellid = []
    time = []
    throughput = []
    timeT = []


    with open('DlRsrpSinrStats.txt') as f:
        lines = f.readlines()
        for i in lines[1:]:
            time.append(float(i.split('\t')[0]))
            sinr.append(10 * log10(float(i.split('\t')[5])))
            rsrp.append(10 * log10(float(i.split('\t')[4])))
            cellid.append(float(i.split('\t')[1]))

        cell = 1
        ho_times = []
        for t, c in zip(time, cellid):
            if(cell != c):
                ho_times.append(t)
            cell = c

        with open('throughput.out', 'r') as f:
            lines = f.readlines()
            for i in lines:
                try:
                    throughput.append(float(i.split()[3]))
                    timeT.append(float(i.split()[0]))
                except ValueError:
                    pass
            for i in ho_times:
                plt.axvline(i, color='r')
            plt.plot(timeT, throughput)
            plt.xlabel("Time (s)")
            plt.ylabel("Throughput Mbps")
            plt.grid(True)

        plt.figure()
        plt.grid(True)
        plt.ylabel('Connected Cell')
        plt.xlabel('Time (s)')
        plt.ylim(0.5, 4.5)
        plt.xlim(0, 150)
        plt.yticks(np.arange(1, 5), np.arange(1, 5))
        for i in ho_times:
            plt.axvline(i, color='r')
        plt.plot(time, cellid)

        plt.figure()
        for i in ho_times:
            plt.axvline(i, color='r')
        plt.grid(True)
        plt.plot(time, sinr)
        plt.xlim(0, 150)
        plt.xlabel('Time (s)')
        plt.ylabel('SINR (dBm)')

        plt.figure()
        for i in ho_times:
            plt.axvline(i, color='r')
        plt.grid(True)
        plt.plot(time, rsrp)
        plt.xlim(0, 150)
        plt.xlabel('Time (s)')
        plt.ylabel('Received Power (dBm)')

        plt.show()
if __name__=="__main__":
    main()
