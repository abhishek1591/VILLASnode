import csv
import sys
import numpy as np
import matplotlib.pyplot as plt
import re

# check if called correctly
if len(sys.argv) < 2:
	sys.exit('Usage: %s FILE1 FILE2' % sys.argv[0])

plt.figure(figsize=(8,4))

for fn in sys.argv[1:]:

#	m = re.match('[a-zA-Z-]+[-_](\d+)[-_](\d+).', fn)
#	rate = m.group(1)
#	values = m.group(2)
#	print 'Processing file %s (rate=%s, values=%s)' % (fn, rate, values)

	# read data from file
	data = [ ]
	with open(fn) as f:
		reader = csv.reader(f, delimiter='\t')

		for row in reader:
			offset = float(row[0])

#			if fn != 'nrel-test1_offset.log':
#				offset = offset * 0.001

			if offset > 100:
				continue

			data.append(offset)

	# evaluate the histogram
	values, base = np.histogram(data, bins='fd')

	# evaluate the cumulative
	cumulative = np.cumsum(values)
	cumscaled = [ float(x) / len(data) for x in cumulative ]

	# plot the cumulative function
	plt.plot(base[:-1], cumscaled, label=fn, linewidth=1)

	# plot the distribution
	#valscaled = [ float(x) / len(data) for x in values ]
	#plt.plot(base[:-1], valscaled, label=fn, linewidth=1)

plt.xlabel('RTT (s)')
plt.ylabel('Cum. Probability')
plt.grid(color='0.75')

#plt.yscale('log')

#plt.ylim([0, 1.03])
#plt.xlim([0.025, 0.05])

lgd = plt.legend(title='Rate (p/s)', loc='center left', bbox_to_anchor=(1, 0.5))

plt.show()

#plt.savefig('cumdist.png', dpi=600, bbox_extra_artists=(lgd,), bbox_inches='tight')
