import pandas as pd
import matplotlib.pyplot as plt
import os, sys


fig, ax = plt.subplots(figsize=(10,5))

default = pd.read_csv("results/default/tiny/default.csv")
t1 = pd.read_csv("results/16Buffer/tiny/kmap.csv")
t2 = pd.read_csv("results/24_w_pthread/tiny/kmap.csv")


merged = pd.DataFrame()
merged['16B'] = t1['Times']
merged['24B'] = t2['Times']
merged['sys'] = default['Times']

bplot = ax.boxplot(x=merged.values, showfliers=False, notch=False, widths=0.5, patch_artist=True)
bplot['boxes'][0].set_facecolor('orange')
bplot['boxes'][1].set_facecolor('purple')
bplot['boxes'][2].set_facecolor('grey')
for _, box in enumerate(bplot['medians']):
    box.set_color('black')
    box.set_linewidth(2.0)

for _, box in enumerate(bplot['whiskers']):
    box.set_color('black')
    box.set_linewidth(2.0)

for _, box in enumerate(bplot['caps']):
    box.set_color('black')
    box.set_linewidth(2.0)

ax.set_xlabel('')
ax.set_ylabel('Time (s)')

ax.set_title('Comparing Buffer Sizes')
ax.legend([bplot["boxes"][0], bplot["boxes"][1], bplot["boxes"][2]],
             ['2^16 Buffer', '2^24 Buffer', 'Sys'], loc='upper right', fontsize=12, markerscale=5.0)


plt.savefig('buf_compare.png', dpi=300, bbox_inches = 'tight',pad_inches = 0)