from src.logger import LOG

def create_string_array(values):
    return f"[{','.join(str(x) for x in values)}]"

def create_distribution_script(*, values, title, save_path):
    values_string = create_string_array(values)
    output = f'''
import numpy as np
import matplotlib.pyplot as plt

a = np.hstack({values_string})
plt.hist(a, bins='auto')  # arguments are passed to np.histogram
plt.title("{title}")
plt.show()'''

    fh = open(save_path, 'w')
    fh.write(output)
    fh.close()

    LOG.info(f'Run: {save_path}, to look at {title} distribution.')


def create_plots(*, plots, xlabel, ylabel, save_path):
    legends = "["
    xlist = "["
    ylist = "["
    for plot in plots:
        legends += "'" + plot['legend'] + "'" + ','
        xlist += create_string_array(plot['points_x']) + ','
        ylist += create_string_array(plot['points_y']) + ','
    legends += ']'
    xlist += ']'
    ylist += ']'

    output = f'''
import pylab

legends = {legends}
xlist = {xlist}
ylist = {ylist}
for (x, y, l) in zip(xlist, ylist, legends):
  pylab.plot(x, y, label=l)

pylab.axhline(0, color='red', marker='o', linestyle='dashed')

pylab.xlabel("{xlabel}")
pylab.ylabel("{ylabel}")
pylab.legend()
pylab.tight_layout()
pylab.show()
'''
    fh = open(save_path, 'w')
    fh.write(output)
    fh.close()

    LOG.info(f'Run: {save_path}, to look at {xlabel} x {ylabel} plot.')
